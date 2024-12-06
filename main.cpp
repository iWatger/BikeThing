#include "mbed.h"
#include "mbedlibrarycollection/libraries/ulcd/uLCD.hpp"

DigitalIn left_button(p29);
DigitalIn right_button(p26);
DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
AnalogIn   brake(p20);

BufferedSerial serial(p28, p27, 9600);

uLCD screen(p9, p10, p8, uLCD::BAUD_115200);
#define BLUE screen.get4DGLColor("0000FF")
#define BLACK screen.get4DGLColor("000000")
#define RED screen.get4DGLColor("FF0000")
// main() runs in its own thread in the OS


//Draw and Clear Arrows on the ULCD
void drawArrows(bool left, bool right) {
    //Right arrow
    if (right) {
        screen.drawTriangle(125, 70, 100, 45, 100, 95, BLUE);
        screen.drawRectangle(100, 55, 75, 85, BLUE);
    } else {
        screen.drawTriangle(125, 70, 100, 45, 100, 95, BLACK);
        screen.drawRectangle(100, 55, 75, 85, BLACK);
    }
    if (left) {
        screen.drawTriangle(5, 70, 30, 45, 30, 95, BLUE);
        screen.drawRectangle(30, 55, 55, 85, BLUE);
    } else {
        screen.drawTriangle(5, 70, 30, 45, 30, 95, BLACK);
        screen.drawRectangle(30, 55, 55, 85, BLACK);
    }
}

// Draw Blind Spot detection 
void drawBLIS(bool blis) {
    if (blis) {
        screen.drawRectangleFilled(0, 0, 128, 128, RED);
    }
}

Timer timer;
int left_milli = 0;
int right_milli = 0;
bool isLeft = false;
bool isRight = false;
bool isBLIS = false;
bool isBrake = false;
int prev_left = 0;
int prev_right = 0;
int prevmsg = 0;

int main()
{
    printf("Start %f\n", brake.read());
    char buffer[3];
    timer.start();
    serial.set_blocking(true);

    while (true) {
        printf("Level: %f\n", brake.read());

        // get brake input from the force sensor
        isBrake = brake.read() < 0.67 ? 1 : 0;

        // if the left button is pressed and the debounce is over, register a press
        if (left_button && timer.elapsed_time().count() / 1000 > prev_left + 1000) {
            isLeft = true;
            printf("Left\n");
            prev_left = timer.elapsed_time().count() / 1000;   
        }
        // if the right button is pressed and the debounce is over, register a press
        if (right_button && timer.elapsed_time().count() / 1000 > prev_right + 1000) {
            isRight = true;
            printf("Right\n");
            prev_right = timer.elapsed_time().count() / 1000;
        }

        // after 10 seconds, flip the turn signal off
        if (isLeft && timer.elapsed_time().count() / 1000 > prev_left + 10000) {
            isLeft = false;
        }
        // after 10 seconds, flip the turn signal off
        if (isRight && timer.elapsed_time().count() / 1000 > prev_right + 10000) {
            isRight = false;
        }

        // draw arrows with the given state
        drawArrows(isLeft, isRight);
        
        led1 = isLeft;
        led2 = isBrake;
        led3 = isRight;
        // every quarter second, send the current brake and turn signal data to arduino
        if (timer.elapsed_time().count() / 1000 > prevmsg + 250) {
            int buf = 0;
            buf |= isLeft ? 1 << 0 : 0;
            buf |= isBrake ? 1 << 1 : 0;
            buf |= isRight ? 1 << 2 : 0;
            printf("Sending %d\n", buf);
            serial.write(&buf, 1);
            prevmsg = timer.elapsed_time().count() / 1000;
        }
    }
}