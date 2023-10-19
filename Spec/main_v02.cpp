#include <Arduino.h>

#define DAC_PIN 25         // GPIO pin connected to the DAC (you can change this)
#define ADC_PIN 34         // GPIO pin connected to the ADC for echo signal (you can change this)
#define SAMPLE_RATE 400000 // Nyquist criteria: twice the highest frequency (200 kHz)
#define DURATION 0.01      // Chirp duration in seconds

void setup() {
  // Initialize the DAC
  dac_output_enable(DAC_CHANNEL_1);

  // Initialize the ADC
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_0);
}

void loop() {
  // Generate an 80 kHz chirp signal
  generateChirpSignal(DAC_PIN, 80e3, DURATION);

  // Capture and process the received signal for Doppler analysis
  float velocity_80kHz = processEchoSignal(80e3, DURATION);

  delay(1000); // Delay for 1 second between chirp signals

  // Generate a 200 kHz chirp signal
  generateChirpSignal(DAC_PIN, 200e3, DURATION);

  // Capture and process the received signal for Doppler analysis
  float velocity_200kHz = processEchoSignal(200e3, DURATION);

  delay(1000); // Delay for 1 second before the next chirp signal

  // Use velocity_80kHz and velocity_200kHz for further processing or analysis
}

void generateChirpSignal(int pin, double f_end, double duration) {
  int sample_rate = 2 * f_end; // Nyquist criterion: Sample rate at least twice the highest frequency
  int num_samples = sample_rate * duration;
  double t_step = 1.0 / sample_rate;
  double t = 0.0;
  double f_start = 10e3; // Start frequency in Hz (10 kHz)
  
  for (int i = 0; i < num_samples; i++) {
    double frequency = f_start + (f_end - f_start) * t / duration;
    double amplitude = 127.5 * sin(2.0 * PI * frequency * t) + 127.5;
    dac_output_voltage(DAC_CHANNEL_1, (uint8_t)amplitude);
    
    delayMicroseconds(1000000 * t_step);
    t += t_step;
  }
  // Turn off the DAC output
  dac_output_voltage(DAC_CHANNEL_1, 0);
}

float processEchoSignal(double expected_frequency, double duration) {
  int num_samples = SAMPLE_RATE * duration;
  float velocity = 0.0;
  float max_correlation = 0.0; // Keep track of the maximum cross-correlation

  // Generate a reference chirp signal
  float t_ref = 0.0;
  float t_step_ref = duration / num_samples;
  int ref_size = num_samples;
  int16_t reference_signal[ref_size];

  for (int i = 0; i < ref_size; i++) {
    double frequency = 10e3 + (expected_frequency - 10e3) * t_ref / duration;
    reference_signal[i] = 127.5 * sin(2.0 * PI * frequency * t_ref) + 127.5;
    t_ref += t_step_ref;
  }

  for (int i = 0; i < num_samples; i++) {
    int adc_value = adc1_get_raw(ADC1_CHANNEL_6);
    float correlation = 0.0;

    // Perform matched filtering (cross-correlation)
    for (int j = 0; j < ref_size; j++) {
      if (i + j < num_samples) {
        correlation += adc_value * reference_signal[j];
      }
    }

    // Update velocity based on the peak in the cross-correlation
    if (correlation > max_correlation) {
      max_correlation = correlation;
      velocity = (expected_frequency - 10e3) * duration / (2.0 * PI * j * t_step_ref);
    }
  }

  return velocity;
}
