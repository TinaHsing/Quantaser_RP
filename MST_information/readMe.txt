用到的檔案:

CH1_CMD = "LD_LIBRARY_PATH=/opt/quantaser/lib ./CH1 "
由single_ch1.c 產生

DAC8_CMD = "LD_LIBRARY_PATH=/opt/quantaser/lib ./DAC 8 "
由dac_single_v2.c產生

MST1_CMD = "LD_LIBRARY_PATH=/opt/quantaser/lib ./MST_CH1 "
MST2_CMD = "LD_LIBRARY_PATH=/opt/quantaser/lib ./MST_CH2 "
由MST_v3.c產生，藉由改變#define ADC_CH1or ADC_CH2 來選擇ADC channel

ADC_MV_CH1_READ = "LD_LIBRARY_PATH=/opt/quantaser/lib ./ADC_MV_CH1 "
ADC_MV_CH2_READ = "LD_LIBRARY_PATH=/opt/quantaser/lib ./ADC_MV_CH2 "
由adc_MV_FPGA.c產生，藉由改變#define ADC_CH1or ADC_CH2 來選擇ADC channel