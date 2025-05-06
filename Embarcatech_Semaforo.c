#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/pio.h"
#include "ws2818b.pio.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"

#define GREEN_LED 11
#define RED_LED 13
#define BUTTON_A 5
#define BUZZER 21
#define I2C_PORT i2c1
#define DISPLAY_SDA 14
#define DISPLAY_SCL 15
#define ENDERECO 0x3C
#define LED_COUNT 25
#define LED_PIN 13
#define MATRIX_PIN 7
#define A4 440
#define A3 220
#define A5 880
#define E4 330

bool display_color = true;
bool mode_night = false;
uint32_t last_time = 0;
const uint FREQUENCY = 50;   // Frequência do PWM
const uint WRAP = (1000000 / FREQUENCY);  // Período em microssegundos
char traffic_light[10];
ssd1306_t ssd;

struct pixel_t {
    uint8_t G, R, B;        // Componentes de cor: Verde, Vermelho e Azul
};

typedef struct pixel_t pixel_t; // Alias para a estrutura pixel_t
typedef pixel_t npLED_t;        // Alias para facilitar o uso no contexto de LEDs

npLED_t leds[LED_COUNT];        // Array para armazenar o estado de cada LED
PIO np_pio;                     // Variável para referenciar a instância PIO usada
uint sm;                        // Variável para armazenar o número do state machine usado

// Função para calcular o índice do LED na matriz
int getIndex(int x, int y);

// Função para inicializar o PIO para controle dos LEDs
void npInit(uint pin);

// Função para definir a cor de um LED específico
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b);

// Função para limpar (apagar) todos os LEDs
void npClear();

// Função para atualizar os LEDs no hardware
void npWrite();

void init_pwm(uint gpio);

void set_buzzer_tone(uint gpio, uint freq);

void stop_buzzer(uint gpio);

void standard_mode();

void night_mode();

void button_callback(uint gpio, uint32_t event);

void vSemaforoTask(){
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, button_callback);
    while (true){
        
        if (!mode_night){
            standard_mode();
        } else {
            night_mode();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void vUpdateDisplayTask(){
    while (true) {
        ssd1306_fill(&ssd, !display_color);
        ssd1306_draw_string(&ssd, "Modo Noturno", 10, 10);
        if (mode_night){
            ssd1306_draw_string(&ssd, "Ativado", 20, 20);
        } else{
            ssd1306_draw_string(&ssd, "Desativado", 20, 20);
        }
        ssd1306_draw_string(&ssd, "Semaforo", 10, 30);
        ssd1306_draw_string(&ssd, traffic_light, 10, 40);
        if (strcmp(traffic_light, "verde") == 0){
            ssd1306_draw_string(&ssd, "Aguarde", 40, 50);
        }else if (strcmp(traffic_light, "amarelo") == 0){
            ssd1306_draw_string(&ssd, "Atencao", 40, 50);
        } else{
            ssd1306_draw_string(&ssd, "Atravesse", 40, 50);
        }
        ssd1306_send_data(&ssd);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void vUpdateMatrixTask() {
    const uint8_t total_leds = 25;
    TickType_t delay_per_led = 0;
    char current_state[10] = "";

    while (true) {
        TickType_t green_duration = pdMS_TO_TICKS(10000);   // 10s
        TickType_t yellow_duration = mode_night ? pdMS_TO_TICKS(1000) : pdMS_TO_TICKS(3000);
        TickType_t red_duration = pdMS_TO_TICKS(10000);

        // Se o estado mudou, reinicia a matriz
        if (strcmp(current_state, traffic_light) != 0) {
            strcpy(current_state, traffic_light);
            npClear();

            if (strcmp(current_state, "verde") == 0) {
                delay_per_led = green_duration / total_leds;
            } else if (strcmp(current_state, "amarelo") == 0) {
                delay_per_led = yellow_duration / total_leds;
            } else {
                delay_per_led = red_duration / total_leds;
            }
        }

        // Acende progressivamente
        if (delay_per_led > 0) {
            for (uint8_t i = 0; i < total_leds; ++i) {
                if (strcmp(traffic_light, current_state) != 0) break;

                if (strcmp(current_state, "amarelo") == 0) {
                    npSetLED(i, 200, 200, 0);
                } else if (strcmp(current_state, "verde") == 0) {
                    npSetLED(i, 0, 200, 0);
                } else {
                    npSetLED(i, 200, 0, 0);
                }
                npWrite();
                vTaskDelay(delay_per_led);
            }
            if (mode_night) {
                npClear();
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}



void display_init(){
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO, I2C_PORT); // Inicialização do display
    ssd1306_config(&ssd); // Configura display
    ssd1306_send_data(&ssd); // envia dados para o display
    ssd1306_fill(&ssd, false); // limpa o display
    ssd1306_send_data(&ssd);
}

void hardware_config(){
    gpio_init(GREEN_LED);
    gpio_init(RED_LED);
    gpio_set_dir(GREEN_LED, GPIO_OUT);
    gpio_set_dir(RED_LED, GPIO_OUT);
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    init_pwm(BUZZER);
    gpio_set_function(DISPLAY_SDA, GPIO_FUNC_I2C);
    gpio_set_function(DISPLAY_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(DISPLAY_SDA);
    gpio_pull_up(DISPLAY_SCL);
    i2c_init(I2C_PORT, 400e3);
    display_init();
    npInit(MATRIX_PIN);
}

int main()
{
    stdio_init_all();
    
    hardware_config();

    xTaskCreate(vSemaforoTask, "Semaforo Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vUpdateDisplayTask, "Update Display Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vUpdateMatrixTask, "Update Matrix Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    vTaskStartScheduler();
    panic_unsupported(); // Nunca deve chegar aqui
}

void init_pwm(uint gpio) {
    gpio_set_function(gpio, GPIO_FUNC_PWM); // Configura o GPIO como PWM
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    pwm_set_clkdiv(slice_num, 125.0f);     // Define o divisor do clock para 1 MHz
    pwm_set_wrap(slice_num, 1000);        // Define o TOP para frequência de 1 kHz
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(gpio), 0); // Razão cíclica inicial
    pwm_set_enabled(slice_num, true);     // Habilita o PWM
}

void set_buzzer_tone(uint gpio, uint freq) {
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    uint top = 1000000 / freq;            // Calcula o TOP para a frequência desejada
    pwm_set_wrap(slice_num, top);
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(gpio), top / 4); // 50% duty cycle
}

void stop_buzzer(uint gpio) {
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(gpio), 0); // Desliga o PWM
}

void standard_mode(){
    gpio_put(RED_LED, false);
    gpio_put(GREEN_LED, true);
    strcpy(traffic_light, "verde");
    for (int i=0; i<5; i++){
        set_buzzer_tone(BUZZER, A4);
        vTaskDelay(pdMS_TO_TICKS(1000));
        stop_buzzer(BUZZER);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    gpio_put(RED_LED, true);
    strcpy(traffic_light, "amarelo");
    for (int i=0; i<5; i++){
        set_buzzer_tone(BUZZER, A5);
        vTaskDelay(pdMS_TO_TICKS(300));
        stop_buzzer(BUZZER);
        vTaskDelay(pdMS_TO_TICKS(300));
    }
    gpio_put(GREEN_LED, false);
    strcpy(traffic_light, "vermelho");
    for (int i=0; i<5; i++){
        set_buzzer_tone(BUZZER, A3);
        vTaskDelay(pdMS_TO_TICKS(500));
        stop_buzzer(BUZZER);
        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}

void night_mode(){
    strcpy(traffic_light, "amarelo");
    gpio_put(GREEN_LED, true);
    gpio_put(RED_LED, true);
    set_buzzer_tone(BUZZER, E4);
    vTaskDelay(pdMS_TO_TICKS(1000));
    gpio_put(GREEN_LED, false);
    gpio_put(RED_LED, false);
    stop_buzzer(BUZZER);
    vTaskDelay(pdMS_TO_TICKS(1000));
}

void button_callback(uint gpio, uint32_t event){
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - last_time > 200){
        mode_night = !mode_night;
        printf("Modo Noturno: ");
        printf(mode_night?"Ligado\n":"Desligado\n");
        last_time = current_time;
    }
}

int getIndex(int x, int y) {
    x = 4 - x; // Inverte as colunas (0 -> 4, 1 -> 3, etc.)
    y = 4 - y; // Inverte as linhas (0 -> 4, 1 -> 3, etc.)
    if (y % 2 == 0) {
        return y * 5 + x;       // Linha par (esquerda para direita)
    } else {
        return y * 5 + (4 - x); // Linha ímpar (direita para esquerda)
    }
}

void npInit(uint pin) {
    uint offset = pio_add_program(pio0, &ws2818b_program); // Carregar o programa PIO
    np_pio = pio0;                                         // Usar o primeiro bloco PIO

    sm = pio_claim_unused_sm(np_pio, false);              // Tentar usar uma state machine do pio0
    if (sm < 0) {                                         // Se não houver disponível no pio0
        np_pio = pio1;                                    // Mudar para o pio1
        sm = pio_claim_unused_sm(np_pio, true);           // Usar uma state machine do pio1
    }

    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f); // Inicializar state machine para LEDs

    for (uint i = 0; i < LED_COUNT; ++i) {                // Inicializar todos os LEDs como apagados
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }
}

void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
    leds[index].R = r;                                    // Definir componente vermelho
    leds[index].G = g;                                    // Definir componente verde
    leds[index].B = b;                                    // Definir componente azul
}

void npClear() {
    for (uint i = 0; i < LED_COUNT; ++i) {                // Iterar sobre todos os LEDs
        npSetLED(i, 0, 0, 0);                             // Definir cor como preta (apagado)
    }
    npWrite();                                            // Atualizar LEDs no hardware
}

void npWrite() {
    for (uint i = 0; i < LED_COUNT; ++i) {                // Iterar sobre todos os LEDs
        pio_sm_put_blocking(np_pio, sm, leds[i].G);       // Enviar componente verde
        pio_sm_put_blocking(np_pio, sm, leds[i].R);       // Enviar componente vermelho
        pio_sm_put_blocking(np_pio, sm, leds[i].B);       // Enviar componente azul
    }
}