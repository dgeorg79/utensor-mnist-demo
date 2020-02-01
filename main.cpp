#include "mbed.h"
#include "stm32f413h_discovery.h"
#include "stm32f413h_discovery_ts.h"
#include "stm32f413h_discovery_lcd.h"
#include "tensor.hpp"
#include "image.h"
#include "models/deep_mlp.hpp"

Serial pc(USBTX, USBRX, 115200);

#ifdef TARGET_SIMULATOR
#define USER_BUTTON     BUTTON1
#endif

InterruptIn button(USER_BUTTON);

TS_StateTypeDef  TS_State = {0};

volatile bool trigger_inference = false;

void trigger_inference_cb(void){ trigger_inference = true; }

template<typename T>
void clear(Image<T>& img){
    for(int i = 0; i < img.get_xDim(); i++){
        for(int j = 0; j < img.get_yDim(); j++){
            img(i,j) = 0;
        }
    }
}

template<typename T>
void printImage(const Image<T>& img){

    for(int i = 0; i < img.get_xDim(); i++){
        for(int j = 0; j < img.get_yDim(); j++){
            printf("%f, ", img(i,j));
        }
        printf("]\n\r");
    }
}

int main()
{
    uint16_t x1, y1;
    printf("uTensor deep learning character recognition demo\n");
    printf("https://github.com/uTensor/utensor-mnist-demo\n");
    printf("Draw a number (0-9) on the touch screen, and press the button...\r\n");


    Image<float>* img = new Image<float>(240, 240);

    BSP_LCD_Init();
    button.rise(&trigger_inference_cb);

    /* Touchscreen initialization */
    if (BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize()) == TS_ERROR) {
        printf("BSP_TS_Init error\n");
    }

    /* Clear the LCD */
    BSP_LCD_Clear(LCD_COLOR_WHITE);

    Context ctx;
    clear(*img);


    while (1) {
        BSP_TS_GetState(&TS_State);
        if(trigger_inference){
          
            Image<float> smallImage = resize(*img, 28, 28);

            pc.printf("Done padding\n\n");
            delete img;

            pc.printf("Reshaping\n\r");
            smallImage.get_data()->resize({1, 784});
            pc.printf("Creating Graph\n\r");

            get_deep_mlp_ctx(ctx, smallImage.get_data());
            pc.printf("Evaluating\n\r");
            ctx.eval();
            S_TENSOR prediction = ctx.get({"y_pred:0"});
            int result = *(prediction->read<int>(0,0));

            printf("Number guessed %d\n\r", result);

            BSP_LCD_Clear(LCD_COLOR_WHITE);
            BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
            BSP_LCD_SetFont(&Font24);

            // Create a cstring
            uint8_t number[2];
            number[1] = '\0';
            //ASCII numbers are 48 + the number, a neat trick
            number[0] = 48 + result;
            BSP_LCD_DisplayStringAt(0, 120, number, CENTER_MODE);
            trigger_inference = false;
            exit(0);
        }
        if(TS_State.touchDetected) {
            /* One or dual touch have been detected          */

            /* Get X and Y position of the first touch post calibrated */
            x1 = TS_State.touchX[0];
            y1 = TS_State.touchY[0];

            img->draw_circle(x1, y1, 7); //Screen not in image x,y format. Must transpose

            BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
            BSP_LCD_FillCircle(x1, y1, 5);

            wait_ms(5);
        }
    }
}
