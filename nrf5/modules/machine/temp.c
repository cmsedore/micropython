/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Bander F. Ajba
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
#include <string.h>

#include "py/nlr.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "temp.h"
#include "hal_temp.h"

#if MICROPY_PY_MACHINE_TEMP

typedef struct _machine_temp_obj_t {
    mp_obj_base_t base;
} machine_temp_obj_t;

/// \method __str__()
/// Return a string describing the Temp object.
STATIC void machine_temp_print(const mp_print_t *print, mp_obj_t o, mp_print_kind_t kind) {
    machine_temp_obj_t *self = o;

    (void)self;

    mp_printf(print, "Temp");
}

/******************************************************************************/
/* MicroPython bindings for machine API                                       */

STATIC mp_obj_t machine_temp_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    static const mp_arg_t allowed_args[] = {
        { },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    
    machine_temp_obj_t *self = m_new_obj(machine_temp_obj_t);
    
    self->base.type = &machine_temp_type;

    return MP_OBJ_FROM_PTR(self);
}

/// \method read()
/// Get temperature.
STATIC mp_obj_t machine_temp_read(mp_uint_t n_args, const mp_obj_t *args) {

    return MP_OBJ_NEW_SMALL_INT(hal_temp_read());
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_machine_temp_read_obj, 0, 1, machine_temp_read);

STATIC const mp_rom_map_elem_t machine_temp_locals_dict_table[] = {
    // instance methods
    // class methods
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_machine_temp_read_obj) },
};

STATIC MP_DEFINE_CONST_DICT(machine_temp_locals_dict, machine_temp_locals_dict_table);

const mp_obj_type_t machine_temp_type = {
    { &mp_type_type },
    .name = MP_QSTR_Temp,
    .make_new = machine_temp_make_new,
    .locals_dict = (mp_obj_dict_t*)&machine_temp_locals_dict,
    .print = machine_temp_print,
};

#endif // MICROPY_PY_MACHINE_TEMP