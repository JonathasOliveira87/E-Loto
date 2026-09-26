#include "Touch.h"
#include "Hardware.h"
#include "Estado.h"
#include "Telas.h"
#include <Arduino.h>

void testarTouchDireto()
{
    controladorTouch.read();


    if (
        controladorTouch.isTouched &&
        controladorTouch.touches > 0
    )
    {
        ultimoToqueMillis =
            millis();
    }
}

void my_touch_read(
    lv_indev_t *indev_driver,
    lv_indev_data_t *data
)
{
    controladorTouch.read();


    if (
        controladorTouch.isTouched &&
        controladorTouch.touches > 0
    )
    {
        int rawX =
            controladorTouch.points[0].x;


        int rawY =
            controladorTouch.points[0].y;


        int x =
            rawY;


        int y =
            (ALTURA_TELA - 1) -
            rawX;


        x = constrain(
            x,
            0,
            LARGURA_TELA - 1
        );


        y = constrain(
            y,
            0,
            ALTURA_TELA - 1
        );


        data->point.x =
            x;


        data->point.y =
            y;


        data->state =
            LV_INDEV_STATE_PRESSED;


        ultimoToqueMillis =
            millis();
    }
    else
    {
        data->state =
            LV_INDEV_STATE_RELEASED;
    }
}

