/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2015 Damien P. George
 * Copyright (c) 2016 Glenn Ruben Bakke
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <stdio.h>

#include "modmachine.h"
#include "py/gc.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "extmod/machine_mem.h"
#include "extmod/machine_pulse.h"
#include "extmod/machine_i2c.h"
#include "lib/utils/pyexec.h"
#include "lib/fatfs/ff.h"
#include "lib/fatfs/diskio.h"
#include "gccollect.h"
#include "pin.h"
#include "spi.h"
#include "pwm.h"

#define PYB_RESET_HARD      (0)
#define PYB_RESET_WDT       (1)
#define PYB_RESET_SOFT      (2)
#define PYB_RESET_LOCKUP    (3)
#define PYB_RESET_POWER_ON  (16)
#define PYB_RESET_LPCOMP    (17)
#define PYB_RESET_DIF       (18)
#define PYB_RESET_NFC       (19)

STATIC uint32_t reset_cause;

void machine_init(void) {
    uint32_t state = NRF_POWER->RESETREAS;
    if (state & POWER_RESETREAS_RESETPIN_Msk) {
        reset_cause = PYB_RESET_HARD;
    } else if (state & POWER_RESETREAS_DOG_Msk) {
        reset_cause = PYB_RESET_WDT;
    } else if (state & POWER_RESETREAS_SREQ_Msk) {
        reset_cause = PYB_RESET_SOFT;
    } else if (state & POWER_RESETREAS_LOCKUP_Msk) {
        reset_cause = PYB_RESET_LOCKUP;
    } else if (state & POWER_RESETREAS_OFF_Msk) {
        reset_cause = PYB_RESET_POWER_ON;
    } else if (state & POWER_RESETREAS_LPCOMP_Msk) {
        reset_cause = PYB_RESET_LPCOMP;
    } else if (state & POWER_RESETREAS_DIF_Msk) {
        reset_cause = PYB_RESET_DIF;
#if NRF52
    } else if (state & POWER_RESETREAS_NFC_Msk) {
        reset_cause = PYB_RESET_NFC;
#endif
    }

    // clear reset reason
    NRF_POWER->RESETREAS = (1 << reset_cause);
}

// machine.info([dump_alloc_table])
// Print out lots of information about the board.
STATIC mp_obj_t machine_info(mp_uint_t n_args, const mp_obj_t *args) {
    // to print info about memory
    {
        printf("_etext=%p\n", &_etext);
        printf("_sidata=%p\n", &_sidata);
        printf("_sdata=%p\n", &_sdata);
        printf("_edata=%p\n", &_edata);
        printf("_sbss=%p\n", &_sbss);
        printf("_ebss=%p\n", &_ebss);
        printf("_estack=%p\n", &_estack);
        printf("_ram_start=%p\n", &_ram_start);
        printf("_heap_start=%p\n", &_heap_start);
        printf("_heap_end=%p\n", &_heap_end);
        printf("_ram_end=%p\n", &_ram_end);
    }

    // qstr info
    {
        mp_uint_t n_pool, n_qstr, n_str_data_bytes, n_total_bytes;
        qstr_pool_info(&n_pool, &n_qstr, &n_str_data_bytes, &n_total_bytes);
        printf("qstr:\n  n_pool=" UINT_FMT "\n  n_qstr=" UINT_FMT "\n  n_str_data_bytes=" UINT_FMT "\n  n_total_bytes=" UINT_FMT "\n", n_pool, n_qstr, n_str_data_bytes, n_total_bytes);
    }

    // GC info
    {
        gc_info_t info;
        gc_info(&info);
        printf("GC:\n");
        printf("  " UINT_FMT " total\n", info.total);
        printf("  " UINT_FMT " : " UINT_FMT "\n", info.used, info.free);
        printf("  1=" UINT_FMT " 2=" UINT_FMT " m=" UINT_FMT "\n", info.num_1block, info.num_2block, info.max_block);
    }

    if (n_args == 1) {
        // arg given means dump gc allocation table
        gc_dump_alloc_table();
    }

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_info_obj, 0, 1, machine_info);

// Resets the pyboard in a manner similar to pushing the external RESET button.
STATIC mp_obj_t machine_reset(void) {
    NVIC_SystemReset();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(machine_reset_obj, machine_reset);

STATIC mp_obj_t machine_soft_reset(void) {
    pyexec_system_exit = PYEXEC_FORCED_EXIT;
    nlr_raise(mp_obj_new_exception(&mp_type_SystemExit));
}
MP_DEFINE_CONST_FUN_OBJ_0(machine_soft_reset_obj, machine_soft_reset);

STATIC mp_obj_t machine_sleep(void) {
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(machine_sleep_obj, machine_sleep);

STATIC mp_obj_t machine_deepsleep(void) {
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(machine_deepsleep_obj, machine_deepsleep);

STATIC mp_obj_t machine_reset_cause(void) {
    return MP_OBJ_NEW_SMALL_INT(reset_cause);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_reset_cause_obj, machine_reset_cause);

STATIC const mp_map_elem_t machine_module_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__),            MP_OBJ_NEW_QSTR(MP_QSTR_umachine) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_info),                (mp_obj_t)&machine_info_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_reset),               (mp_obj_t)&machine_reset_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_soft_reset),          (mp_obj_t)&machine_soft_reset_obj },
#if MICROPY_HW_ENABLE_RNG
    { MP_OBJ_NEW_QSTR(MP_QSTR_rng),                 (mp_obj_t)&pyb_rng_get_obj },
#endif
    { MP_OBJ_NEW_QSTR(MP_QSTR_sleep),               (mp_obj_t)&machine_sleep_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_deepsleep),           (mp_obj_t)&machine_deepsleep_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_reset_cause),         (mp_obj_t)&machine_reset_cause_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_Pin),                 (mp_obj_t)&pin_type },
#if MICROPY_PY_MACHINE_SPI
    { MP_OBJ_NEW_QSTR(MP_QSTR_SPI),                 (mp_obj_t)&machine_hard_spi_type },
#endif

#if MICROPY_PY_MACHINE_PWM
    { MP_OBJ_NEW_QSTR(MP_QSTR_PWM),                 (mp_obj_t)&machine_hard_pwm_type },
#endif
    { MP_OBJ_NEW_QSTR(MP_QSTR_HARD_RESET),         MP_OBJ_NEW_SMALL_INT(PYB_RESET_HARD) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_WDT_RESET),          MP_OBJ_NEW_SMALL_INT(PYB_RESET_WDT) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_SOFT_RESET),         MP_OBJ_NEW_SMALL_INT(PYB_RESET_SOFT) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_LOCKUP_RESET),       MP_OBJ_NEW_SMALL_INT(PYB_RESET_LOCKUP) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_PWRON_RESET),        MP_OBJ_NEW_SMALL_INT(PYB_RESET_POWER_ON) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_LPCOMP_RESET),       MP_OBJ_NEW_SMALL_INT(PYB_RESET_LPCOMP) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_DEBUG_IF_RESET),     MP_OBJ_NEW_SMALL_INT(PYB_RESET_DIF) },
#if NRF52
    { MP_OBJ_NEW_QSTR(MP_QSTR_NFC_RESET),          MP_OBJ_NEW_SMALL_INT(PYB_RESET_NFC) },
#endif
};

STATIC MP_DEFINE_CONST_DICT(machine_module_globals, machine_module_globals_table);

const mp_obj_module_t machine_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&machine_module_globals,
};
