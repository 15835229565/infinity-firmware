#include "console.h"
#include "packet.h"
#include "datatypes.h"
#include "ch.h"
#include "hal.h"
#include "stm32f4xx_conf.h"
#include "comm_usb.h"
#include <string.h>
#include <stdio.h>
#include "comm.h"
#include "controller.h"

void console_process_command(char *command)
{
    enum { kMaxArgs = 64 };
    int argc = 0;
    char *argv[kMaxArgs];

    char *p2 = strtok(command, " ");
    while (p2 && argc < kMaxArgs) {
        argv[argc++] = p2;
        p2 = strtok(0, " ");
    }
    if (argc == 0) {
        console_printf("No command received\n");
        console_printf("\r\n");
        return;
    }
    if (strcmp(argv[0], "ping") == 0) {
        console_printf("pong\n");
        console_printf("\r\n");
    }
    else if (strcmp(argv[0], "mem") == 0) {
        size_t n, size;
        n = chHeapStatus(NULL, &size);
        console_printf("core free memory : %u bytes\n", chCoreGetStatusX());
        console_printf("heap fragments   : %u\n", n);
        console_printf("heap free total  : %u bytes\n", size);
        console_printf("\r\n");
    }
    else if (strcmp(argv[0], "threads") == 0) {
        thread_t *tp;
        static const char *states[] = {CH_STATE_NAMES};
        console_printf("    addr    stack prio refs     state           name time    \n");
        console_printf("-------------------------------------------------------------\n");
        tp = chRegFirstThread();
        do {
            console_printf("%.8lx %.8lx %4lu %4lu %9s %14s %lu\n",
                    (uint32_t)tp, (uint32_t)tp->p_ctx.r13,
                    (uint32_t)tp->p_prio, (uint32_t)(tp->p_refs - 1),
                    states[tp->p_state], tp->p_name, (uint32_t)tp->p_time);
            tp = chRegNextThread(tp);
        } while (tp != NULL);
        console_printf("\r\n");
    }
    else if (strcmp(argv[0], "voltage") == 0) {
        float vbus = controller_get_bus_voltage();
        console_printf("Bus voltage: %.2f volts\n", (double)vbus);
        console_printf("\r\n");
    }
    else if (strcmp(argv[0], "usb_override_set") == 0) {
        comm_set_usb_override(true);
        console_printf("Enabling USB control\n");
        console_printf("\r\n");
    }
    else if (strcmp(argv[0], "usb_override_unset") == 0) {
        comm_set_usb_override(false);
        console_printf("Disabling USB control\n");
        console_printf("\r\n");
    }
    else if (strcmp(argv[0], "current_set") == 0) {
        if (argc == 2)
        {
            if (comm_get_usb_override())
            {
                float curr = 0.0;
                sscanf(argv[1], "%f", &curr);
                console_printf("Setting current to %.2f amps\n", (double)curr);
                controller_set_current(curr);
            }
            else
            {
                console_printf("USB control not enabled\n");
            }
        }
        else
        {
            console_printf("Usage: current_set [current]\n");
        }
        console_printf("\r\n");
    }
    else if (strcmp(argv[0], "stop") == 0) {
        console_printf("Stopping motor\n");
        controller_disable();
        console_printf("\r\n");
    }
    else if (strcmp(argv[0], "zero") == 0) {
        console_printf("Finding encoder zero...\n");
        float zero;
        bool inverted;
        bool result = controller_encoder_zero(10.0, &zero, &inverted);
        if (result)
        {
            console_printf("Found zero: %.2f\n", (double)zero);
            if (inverted)
                console_printf("Encoder inverted\n");
            else
                console_printf("Encoder not inverted\n");
        }
        else
        {
            console_printf("Zeroing failed\n");
        }
        console_printf("\r\n");
    }
    else
    {
        console_printf("%s: command not found\n", argv[0]);
        // console_printf("type help for a list of available commands\n");
        console_printf("\r\n");
    }
}

void console_printf(char* format, ...) {
    va_list arg;
    va_start (arg, format);
    int len;
    static char print_buffer[255];

    print_buffer[0] = PACKET_CONSOLE;
    len = vsnprintf(print_buffer+1, 254, format, arg);
    va_end (arg);

    if(len > 0) {
        packet_send_packet((unsigned char*)print_buffer, (len<254)? len+1: 255);
    }
}