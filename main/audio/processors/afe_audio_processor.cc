#include "afe_audio_processor.h"
#include <esp_log.h>
#include <string.h>
#include <esp_partition.h>

#define PROCESSOR_RUNNING 0x01

#define TAG "AfeAudioProcessor"

AfeAudioProcessor::AfeAudioProcessor()
    : afe_data_(nullptr) {
    event_group_ = xEventGroupCreate();
}

void AfeAudioProcessor::Initialize(AudioCodec* codec, int frame_duration_ms, srmodel_list_t* models_list) {
    codec_ = codec;
    frame_samples_ = frame_duration_ms * 16000 / 1000;

    // Pre-allocate output buffer capacity
    output_buffer_.reserve(frame_samples_);

    int ref_num = codec_->input_reference() ? 1 : 0;

    std::string input_format;
    for (int i = 0; i < codec_->input_channels() - ref_num; i++) {
        input_format.push_back('M');
    }
    for (int i = 0; i < ref_num; i++) {
        input_format.push_back('R');
    }

    // 🎯 直接读取 Flash 分区验证数据
    ESP_LOGI(TAG, "🔍 直接读取 model 分区验证烧录是否成功...");
    const esp_partition_t* model_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "model");
    if (model_part != nullptr) {
        uint8_t header[256];
        if (esp_partition_read(model_part, 0, header, sizeof(header)) == ESP_OK) {
            int model_count = (int)(header[0] | (header[1] << 8) | (header[2] << 16) | (header[3] << 24));
            ESP_LOGI(TAG, "📦 Flash 中的模型数量: %d", model_count);
            
            // 读取模型名称（从偏移 4 开始，每个模型名称占 32 字节）
            for (int i = 0; i < model_count && i < 10; i++) {
                char model_name[33] = {0};
                size_t offset = 4 + i * 256;  // 粗略估计
                if (offset + 32 < sizeof(header)) {
                    memcpy(model_name, &header[offset], 32);
                    model_name[32] = '\0';
                    if (strlen(model_name) > 0) {
                        ESP_LOGI(TAG, "   Flash 模型 %d: %s", i, model_name);
                    }
                }
            }
        }
    }
    
    // 强制重新加载模型
    ESP_LOGI(TAG, "🔍 强制从 model 分区重新加载模型（忽略可能的缓存）...");
    
    srmodel_list_t *models = esp_srmodel_init("model");
    if (models == nullptr) {
        ESP_LOGE(TAG, "❌ 从 model 分区加载模型失败！");
        return;
    }
    
    ESP_LOGI(TAG, "✅ ESP-SR 加载的模型数量: %d", models->num);
    // 打印所有模型名称
    for (int i = 0; i < models->num; i++) {
        ESP_LOGI(TAG, "   ESP-SR 模型 %d: %s", i, models->model_name[i]);
    }

    // 🎯 显式指定使用神经网络模型（避免 filter 返回旧版本）
    #ifdef CONFIG_SR_NSN_NSNET2
        char* ns_model_name = esp_srmodel_filter(models, ESP_NSNET_PREFIX, "nsnet2");
        ESP_LOGI(TAG, "🔍 指定降噪模型: nsnet2 (神经网络)");
    #else
        char* ns_model_name = esp_srmodel_filter(models, ESP_NSNET_PREFIX, NULL);
    #endif
    
    #ifdef CONFIG_SR_VADN_VADNET1_MEDIUM
        // 🎯 尝试多个可能的 VADNet1 模型名称（ESP-IDF 5.5 可能使用不同的名称）
        char* vad_model_name = esp_srmodel_filter(models, ESP_VADN_PREFIX, "vadnet1_medium");
        if (vad_model_name == nullptr) {
            ESP_LOGW(TAG, "⚠️  未找到 vadnet1_medium，尝试 vadnet1...");
            vad_model_name = esp_srmodel_filter(models, ESP_VADN_PREFIX, "vadnet1");
        }
        if (vad_model_name == nullptr) {
            ESP_LOGW(TAG, "⚠️  未找到 vadnet1，尝试 vadn1_medium...");
            vad_model_name = esp_srmodel_filter(models, ESP_VADN_PREFIX, "vadn1_medium");
        }
        if (vad_model_name == nullptr) {
            ESP_LOGW(TAG, "⚠️  未找到 vadn1_medium，尝试 vadn1...");
            vad_model_name = esp_srmodel_filter(models, ESP_VADN_PREFIX, "vadn1");
        }
        if (vad_model_name == nullptr) {
            ESP_LOGW(TAG, "⚠️  未找到任何 VADNet 模型，尝试不指定名称（使用第一个匹配的）...");
            vad_model_name = esp_srmodel_filter(models, ESP_VADN_PREFIX, NULL);
        }

        // 🎯 打印所有可用的 VAD 模型，帮助调试
        ESP_LOGI(TAG, "🔍 可用 VAD 模型列表:");
        for (int i = 0; i < models->num; i++) {
            if (strstr(models->model_name[i], "vad") != NULL) {
                ESP_LOGI(TAG, "   [%d] %s", i, models->model_name[i]);
            }
        }

        ESP_LOGI(TAG, "🔍 指定 VAD 模型: vadnet1_medium (神经网络), 找到: %s",
                 vad_model_name ? vad_model_name : "NULL");
    #else
        char* vad_model_name = esp_srmodel_filter(models, ESP_VADN_PREFIX, NULL);
        ESP_LOGI(TAG, "🔍 VAD 模型: 使用默认 (WebRTC)");
    #endif
    
    // 使用低功耗模式，避免 CPU 过载
    // 🎯 传入 models 以确保使用我们指定的 nsnet2 和 vadnet1_medium
    afe_config_t* afe_config = afe_config_init(input_format.c_str(), models, AFE_TYPE_VC, AFE_MODE_LOW_COST);
    // 🛡️ 使用 SR_LOW_COST 模式的 AEC，VOIP 模式太耗 CPU 会触发看门狗
    afe_config->aec_mode = AEC_MODE_SR_LOW_COST;
    
    // 🎯 优化 VAD 参数以更好地检测人声
    afe_config->vad_mode = VAD_MODE_3;  // 最灵敏模式（0=不灵敏, 3=灵敏）
    afe_config->vad_min_noise_ms = 30;   // 进一步缩短噪声判断时间（50ms → 30ms）
    // #region agent log
    ESP_LOGI(TAG, "[DEBUG-CONFIG] VAD 模式: %d, 最小噪声时间: %d ms", 
             afe_config->vad_mode, afe_config->vad_min_noise_ms);
    // #endregion
    if (vad_model_name != nullptr) {
        afe_config->vad_model_name = vad_model_name;
        ESP_LOGI(TAG, "✅ VAD 人声检测: 神经网络模式 (%s, Level 3 高灵敏)", vad_model_name);
    } else {
        ESP_LOGI(TAG, "✅ VAD 人声检测: WebRTC 模式 (Level 3 高灵敏)");
    }

    if (ns_model_name != nullptr) {
        // 使用神经网络降噪模型（现在会被打包）
        afe_config->ns_init = true;
        afe_config->ns_model_name = ns_model_name;
        afe_config->afe_ns_mode = AFE_NS_MODE_NET;
        ESP_LOGI(TAG, "✅ 使用 ESP-SR 神经网络降噪: %s", ns_model_name);
        ESP_LOGI(TAG, "   降噪模式: AFE_NS_MODE_NET (%d)", (int)afe_config->afe_ns_mode);
    } else {
        // 没有 NS 模型，禁用降噪
        afe_config->ns_init = false;
        ESP_LOGW(TAG, "⚠️  未找到 NS 降噪模型，降噪已禁用");
        ESP_LOGW(TAG, "   请运行: idf.py build (会自动打包 NS 模型)");
    }

    // 🎯 启用 AGC（自动增益控制）增强人声
    afe_config->agc_init = true;
    afe_config->agc_mode = AFE_AGC_MODE_WEBRTC;  // 使用 WEBRTC AGC
    afe_config->agc_compression_gain_db = 15;     // 压缩增益 15dB（默认9，越大越激进）
    afe_config->agc_target_level_dbfs = 3;        // 目标电平 -3dBFS（默认3）
    ESP_LOGI(TAG, "✅ AGC 自动增益控制: WEBRTC 模式 (增益=15dB)");
    
    // 🎯 启用 SE（语音增强）突出人声频段
    afe_config->se_init = true;
    ESP_LOGI(TAG, "✅ SE 语音增强: 已启用（突出人声频段，抑制音乐）");
    
    // 🎯 使用 MORE_INTERNAL 策略：关键音频处理用内部RAM（低延迟），大缓冲用PSRAM
    // AFE_MEMORY_ALLOC_MORE_PSRAM 会导致实时性能下降，唤醒后无法正常处理音频
    afe_config->memory_alloc_mode = AFE_MEMORY_ALLOC_MORE_INTERNAL;
    
    // 🎯 大幅增加 AFE Ringbuffer 大小，避免 Speaking 状态下缓冲区溢出
    // VADNet1 神经网络处理更耗时，需要更大的缓冲区防止数据丢失
    afe_config->afe_ringbuf_size = 2000;  // 从 1000 增加到 2000（进一步增大缓冲）
    
    // 🎯 AFE 任务固定到 CPU1（负载较轻的核心）
    // CPU0: audio_input(8) + 图形渲染 → 负载重
    // CPU1: audio_communication + audio_output + LVGL → 相对均衡
    afe_config->afe_perferred_core = 1;  // 固定到 CPU1
    
    // 🎯 提高 AFE 任务优先级，确保及时处理音频数据
    // 优先级 4（中等偏高）：高于播放任务(3)，但不会阻塞系统
    afe_config->afe_perferred_priority = 4;  // 从 2 提升到 4

    // 🎯 初始化时禁用 AEC，只启用 VAD
    // Listening 模式：纯语音识别，不需要 AEC（避免参考通道干扰 VAD）
    // Speaking 模式：播放时需要 AEC（支持打断），由 Start() 动态启用
    afe_config->aec_init = false;  // 初始化时不启用，由 Start() 控制
    afe_config->vad_init = true;   // 始终启用 VAD 用于人声检测
    
    has_reference_ = codec_->input_reference();  // 保存是否有参考通道
    
    if (has_reference_) {
        ESP_LOGI(TAG, "ℹ️  AEC 回声消除: 初始化时禁用（Listening 模式优先 VAD）");
        ESP_LOGI(TAG, "   硬件支持参考通道，可动态启用 AEC（Speaking 模式支持打断）");
    } else {
        ESP_LOGI(TAG, "ℹ️  AEC 回声消除: 硬件不支持参考通道");
    }

    // 🎯 显示优化后的配置
    ESP_LOGI(TAG, "   Ringbuffer 大小: %d, AFE 优先级: %d, AFE 核心: %d",
             afe_config->afe_ringbuf_size, afe_config->afe_perferred_priority, afe_config->afe_perferred_core);
    ESP_LOGI(TAG, "   内存分配模式: %s", 
             afe_config->memory_alloc_mode == AFE_MEMORY_ALLOC_MORE_INTERNAL ? "MORE_INTERNAL" : 
             (afe_config->memory_alloc_mode == AFE_MEMORY_ALLOC_MORE_PSRAM ? "MORE_PSRAM" : "UNKNOWN"));

    ESP_LOGI(TAG, "📦 正在创建 AFE 实例...");
    afe_iface_ = esp_afe_handle_from_config(afe_config);
    if (afe_iface_ == nullptr) {
        ESP_LOGE(TAG, "❌ esp_afe_handle_from_config 失败！");
        return;
    }
    ESP_LOGI(TAG, "✅ AFE handle 创建成功");
    
    afe_data_ = afe_iface_->create_from_config(afe_config);
    if (afe_data_ == nullptr) {
        ESP_LOGE(TAG, "❌ afe_iface_->create_from_config 失败！可能是内存不足");
        return;
    }
    ESP_LOGI(TAG, "✅ AFE data 创建成功");
    
    // 🎯 audio_communication 任务固定到 CPU1（避免 CPU0 过载）
    // 栈大小 4KB，优先级 4：与 audio_output 任务相同
    ESP_LOGI(TAG, "📦 正在创建 audio_communication 任务 (栈: 4KB)...");
    
    // 🎯 修复: FreeRTOS TCB 必须在内部 RAM，只有栈可以在 PSRAM
    TaskHandle_t task_handle = NULL;
    
    // TCB 必须在内部 RAM（FreeRTOS 要求）
    StaticTask_t* task_buffer = (StaticTask_t*)heap_caps_malloc(sizeof(StaticTask_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    // 栈可以在 PSRAM（节省内部 RAM）
    StackType_t* stack_buffer = (StackType_t*)heap_caps_malloc(4096 * sizeof(StackType_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    
    if (task_buffer && stack_buffer) {
        ESP_LOGI(TAG, "✅ TCB 在内部RAM，栈在 PSRAM (4KB)");
        task_handle = xTaskCreateStaticPinnedToCore([](void* arg) {
            ESP_LOGI("AfeAudioProcessor", "🚀 audio_communication 任务已启动！");
            auto this_ = (AfeAudioProcessor*)arg;
            this_->AudioProcessorTask();
            vTaskDelete(NULL);
        }, "audio_communication", 4096, this, 4, stack_buffer, task_buffer, 1);
    } else {
        // 分配失败，回退到内部RAM
        if (task_buffer) free(task_buffer);
        if (stack_buffer) free(stack_buffer);
        ESP_LOGW(TAG, "⚠️  静态分配失败，使用动态分配 (内部 RAM)");
        
        BaseType_t ret = xTaskCreatePinnedToCore([](void* arg) {
            ESP_LOGI("AfeAudioProcessor", "🚀 audio_communication 任务已启动！");
            auto this_ = (AfeAudioProcessor*)arg;
            this_->AudioProcessorTask();
            vTaskDelete(NULL);
        }, "audio_communication", 4096, this, 4, NULL, 1);
        
        if (ret != pdPASS) {
            ESP_LOGE(TAG, "❌ audio_communication 任务创建失败！错误码: %d", ret);
            ESP_LOGE(TAG, "   当前空闲内存: %lu bytes", esp_get_free_heap_size());
            ESP_LOGE(TAG, "   最小空闲内存: %lu bytes", esp_get_minimum_free_heap_size());
        } else {
            ESP_LOGI(TAG, "✅ audio_communication 任务创建成功 (动态分配)");
        }
    }
    
    if (task_handle) {
        ESP_LOGI(TAG, "✅ audio_communication 任务创建成功 (静态分配)");
    }
}

AfeAudioProcessor::~AfeAudioProcessor() {
    if (afe_data_ != nullptr) {
        afe_iface_->destroy(afe_data_);
    }
    vEventGroupDelete(event_group_);
}

size_t AfeAudioProcessor::GetFeedSize() {
    if (afe_data_ == nullptr) {
        ESP_LOGE(TAG, "❌ GetFeedSize: afe_data_ is nullptr!");
        return 0;
    }
    if (afe_iface_ == nullptr) {
        ESP_LOGE(TAG, "❌ GetFeedSize: afe_iface_ is nullptr!");
        return 0;
    }
    return afe_iface_->get_feed_chunksize(afe_data_);
}

void AfeAudioProcessor::Feed(std::vector<int16_t>&& data) {
    if (afe_data_ == nullptr) {
        ESP_LOGW(TAG, "⚠️  AFE data is null, cannot feed");
        return;
    }
    // 检查是否正在运行，避免 Stop 后继续 feed 导致 ringbuffer 溢出
    if ((xEventGroupGetBits(event_group_) & PROCESSOR_RUNNING) == 0) {
        ESP_LOGW(TAG, "⚠️  AFE processor not running, ignoring feed");
        return;
    }
    // 喂数据前短暂延时，给 fetch 任务处理时间，避免 ringbuffer 溢出
    vTaskDelay(pdMS_TO_TICKS(1));
    afe_iface_->feed(afe_data_, data.data());
}

void AfeAudioProcessor::Start() {
    // 🎯 Listening 模式：禁用 AEC，启用 VAD（纯语音识别，参考通道会干扰）
    if (has_reference_ && afe_data_ != nullptr && afe_iface_ != nullptr) {
        ESP_LOGI(TAG, "🎤 Listening 模式：禁用 AEC，启用 VAD（优先语音识别准确度）");
        afe_iface_->disable_aec(afe_data_);
        afe_iface_->enable_vad(afe_data_);
    }
    xEventGroupSetBits(event_group_, PROCESSOR_RUNNING);
}

void AfeAudioProcessor::Stop() {
    xEventGroupClearBits(event_group_, PROCESSOR_RUNNING);
    if (afe_data_ != nullptr) {
        afe_iface_->reset_buffer(afe_data_);
    }
}

bool AfeAudioProcessor::IsRunning() {
    return xEventGroupGetBits(event_group_) & PROCESSOR_RUNNING;
}

void AfeAudioProcessor::OnOutput(std::function<void(std::vector<int16_t>&& data)> callback) {
    output_callback_ = callback;
}

void AfeAudioProcessor::OnVadStateChange(std::function<void(bool speaking)> callback) {
    vad_state_change_callback_ = callback;
}

void AfeAudioProcessor::AudioProcessorTask() {
    auto fetch_size = afe_iface_->get_fetch_chunksize(afe_data_);
    auto feed_size = afe_iface_->get_feed_chunksize(afe_data_);
    ESP_LOGI(TAG, "🚀 Audio communication task started, feed size: %d fetch size: %d",
        feed_size, fetch_size);

    while (true) {
        xEventGroupWaitBits(event_group_, PROCESSOR_RUNNING, pdFALSE, pdTRUE, portMAX_DELAY);

        auto res = afe_iface_->fetch_with_delay(afe_data_, 1000);  // 改为1秒超时，避免永久阻塞
        if (res == nullptr) {
            continue;
        }
        if ((xEventGroupGetBits(event_group_) & PROCESSOR_RUNNING) == 0) {
            continue;
        }
        if (res == nullptr || res->ret_value == ESP_FAIL) {
            if (res != nullptr) {
                ESP_LOGE(TAG, "❌ fetch 失败，错误码: %d", res->ret_value);
            } else {
                ESP_LOGE(TAG, "❌ fetch 返回 nullptr");
            }
            continue;
        }

        // VAD state change
        if (vad_state_change_callback_) {
            if (res->vad_state == VAD_SPEECH && !is_speaking_) {
                is_speaking_ = true;
                vad_state_change_callback_(true);
            } else if (res->vad_state == VAD_SILENCE && is_speaking_) {
                is_speaking_ = false;
                vad_state_change_callback_(false);
            }
        }

        if (output_callback_) {
            size_t samples = res->data_size / sizeof(int16_t);
            
            // Add data to buffer
            output_buffer_.insert(output_buffer_.end(), res->data, res->data + samples);
            
            // Output complete frames when buffer has enough data
            while (output_buffer_.size() >= frame_samples_) {
                if (output_buffer_.size() == frame_samples_) {
                    // If buffer size equals frame size, move the entire buffer
                    output_callback_(std::move(output_buffer_));
                    output_buffer_.clear();
                    output_buffer_.reserve(frame_samples_);
                } else {
                    // If buffer size exceeds frame size, copy one frame and remove it
                    output_callback_(std::vector<int16_t>(output_buffer_.begin(), output_buffer_.begin() + frame_samples_));
                    output_buffer_.erase(output_buffer_.begin(), output_buffer_.begin() + frame_samples_);
                }
            }
        }
    }
}

void AfeAudioProcessor::EnableDeviceAec(bool enable) {
    if (!has_reference_) {
        ESP_LOGW(TAG, "⚠️  硬件不支持 AEC 参考通道，无法切换");
        return;
    }
    
    if (afe_data_ == nullptr || afe_iface_ == nullptr) {
        ESP_LOGW(TAG, "⚠️  AFE 未初始化，无法切换 AEC");
        return;
    }
    
    if (enable) {
        // 🎯 Speaking 模式：启用 AEC，禁用 VAD（支持打断，说话时不录音）
        ESP_LOGI(TAG, "🔊 Speaking 模式：启用 AEC，禁用 VAD（支持打断）");
        afe_iface_->disable_vad(afe_data_);
        afe_iface_->enable_aec(afe_data_);
    } else {
        // 🎯 Listening 模式：禁用 AEC，启用 VAD（纯语音识别）
        ESP_LOGI(TAG, "🎤 Listening 模式：禁用 AEC，启用 VAD（语音识别）");
        afe_iface_->disable_aec(afe_data_);
        afe_iface_->enable_vad(afe_data_);
    }
}
