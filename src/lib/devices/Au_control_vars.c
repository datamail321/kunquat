

/*
 * Author: Tomi Jylhä-Ollila, Finland 2015
 *
 * This file is part of Kunquat.
 *
 * CC0 1.0 Universal, http://creativecommons.org/publicdomain/zero/1.0/
 *
 * To the extent possible under law, Kunquat Affirmers have waived all
 * copyright and related or neighboring rights to Kunquat.
 */


#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <containers/AAtree.h>
#include <debug/assert.h>
#include <devices/Au_control_vars.h>
#include <mathnum/common.h>
#include <memory.h>
#include <string/common.h>
#include <string/Streader.h>
#include <string/var_name.h>
#include <Value.h>


struct Au_control_vars
{
    AAtree* vars;
};


typedef enum
{
    ITER_MODE_DEVICES,
    ITER_MODE_SET_GENERIC,
    ITER_MODE_SET_FLOAT,
    ITER_MODE_OSC_DEPTH_FLOAT,
} Iter_mode;


typedef enum
{
    BIND_ENTRY_TYPE_INVALID,
    BIND_ENTRY_TYPE_EXPRESSION,
    BIND_ENTRY_TYPE_FLOAT_SLIDE,
} Bind_entry_type;


typedef struct Bind_entry
{
    Bind_entry_type type;

    Target_dev_type target_dev_type;
    int target_dev_index;

    Value_type target_var_type;
    char target_var_name[KQT_VAR_NAME_MAX];

    // Type-specific settings
    union
    {
        struct
        {
            char* expression;
        } expr_type;

        struct
        {
            double map_min_to;
            double map_max_to;
        } float_slide_type;
    } ext;

    struct Bind_entry* next;
} Bind_entry;


void del_Bind_entry(Bind_entry* entry)
{
    if (entry == NULL)
        return;

    if (entry->type == BIND_ENTRY_TYPE_EXPRESSION)
        memory_free(entry->ext.expr_type.expression);

    memory_free(entry);

    return;
}


typedef enum
{
    VAR_ENTRY_INVALID,
    VAR_ENTRY_BOOL,
    VAR_ENTRY_INT,
    VAR_ENTRY_FLOAT,
    VAR_ENTRY_TSTAMP,
    VAR_ENTRY_FLOAT_SLIDE,
} Var_entry_type;


typedef struct Var_entry
{
    char name[KQT_VAR_NAME_MAX];
    Var_entry_type type;
    Value init_value;

    // Type-specific settings
    union
    {
        struct
        {
            double min_value;
            double max_value;
        } float_slide_type;
    } ext;

    Bind_entry* first_bind_entry;
    Bind_entry* last_bind_entry;
} Var_entry;


void del_Var_entry(Var_entry* entry)
{
    if (entry == NULL)
        return;

    Bind_entry* cur = entry->first_bind_entry;
    while (cur != NULL)
    {
        Bind_entry* next = cur->next;
        del_Bind_entry(cur);
        cur = next;
    }

    memory_free(entry);

    return;
}


static bool Au_control_binding_iter_init_common(
        Au_control_binding_iter* iter,
        const Au_control_vars* aucv,
        const char* var_name,
        Iter_mode iter_mode)
{
    assert(iter != NULL);
    assert(aucv != NULL);
    assert(var_name != NULL);

    const Var_entry* var_entry = AAtree_get_exact(aucv->vars, var_name);
    if (var_entry == NULL)
        return false;

    iter->iter = var_entry->first_bind_entry;
    if (iter->iter == NULL)
        return false;

    iter->iter_mode = iter_mode;

    return true;
}


static const double range_eps = 0.000001;


static void Au_control_binding_iter_update(Au_control_binding_iter* iter)
{
    assert(iter != NULL);
    assert(iter->iter != NULL);

    iter->target_dev_type = iter->iter->target_dev_type;
    iter->target_dev_index = iter->iter->target_dev_index;
    iter->target_var_name = iter->iter->target_var_name;

    switch (iter->iter_mode)
    {
        case ITER_MODE_DEVICES:
        {
        }
        break;

        case ITER_MODE_SET_GENERIC:
        {
        }
        break;

        case ITER_MODE_SET_FLOAT:
        {
            assert(iter->src_value.type == VALUE_TYPE_FLOAT);

            // Map to target range
            const double map_min_to = iter->iter->ext.float_slide_type.map_min_to;
            const double map_max_to = iter->iter->ext.float_slide_type.map_max_to;
            const double target_value = lerp(
                    map_min_to, map_max_to, iter->ext.set_float_type.src_range_norm);

            iter->target_value.type = VALUE_TYPE_FLOAT;
            iter->target_value.value.float_type = target_value;
        }
        break;

        case ITER_MODE_OSC_DEPTH_FLOAT:
        {
            assert(iter->src_value.type == VALUE_TYPE_FLOAT);

            // Scale oscillation depth to target range
            const double map_min_to = iter->iter->ext.float_slide_type.map_min_to;
            const double map_max_to = iter->iter->ext.float_slide_type.map_max_to;
            const double target_range = map_max_to - map_min_to;
            const double osc_range_norm = iter->ext.osc_float_type.osc_range_norm;
            const double target_depth = target_range * osc_range_norm;

            iter->target_value.type = VALUE_TYPE_FLOAT;
            iter->target_value.value.float_type = target_depth;
        }
        break;

        default:
            assert(false);
    }

    return;
}


bool Au_control_binding_iter_init(
        Au_control_binding_iter* iter,
        const Au_control_vars* aucv,
        const char* var_name)
{
    assert(iter != NULL);
    assert(aucv != NULL);
    assert(var_name != NULL);

    iter->iter = NULL;
    iter->src_value.type = VALUE_TYPE_NONE;

    if (!Au_control_binding_iter_init_common(iter, aucv, var_name, ITER_MODE_DEVICES))
        return false;

    iter->target_value.type = VALUE_TYPE_NONE;

    Au_control_binding_iter_update(iter);

    return true;
}


bool Au_control_binding_iter_init_set_float(
        Au_control_binding_iter* iter,
        const Au_control_vars* aucv,
        const char* var_name,
        double value)
{
    assert(iter != NULL);
    assert(aucv != NULL);
    assert(var_name != NULL);
    assert(isfinite(value));

    iter->iter = NULL;
    iter->src_value.type = VALUE_TYPE_FLOAT;
    iter->src_value.value.float_type = value;

    if (!Au_control_binding_iter_init_common(iter, aucv, var_name, ITER_MODE_SET_FLOAT))
        return false;

    // Get normalised source value position in our source range
    const Var_entry* var_entry = AAtree_get_exact(aucv->vars, var_name);
    assert(var_entry != NULL);

    const double min_value = var_entry->ext.float_slide_type.min_value;
    const double max_value = var_entry->ext.float_slide_type.max_value;
    const double clamped_src_value = clamp(value, min_value, max_value);
    const double src_range = max_value - min_value;
    iter->ext.set_float_type.src_range_norm =
        (src_range > range_eps) ? (clamped_src_value - min_value) / src_range : 0.0;

    Au_control_binding_iter_update(iter);

    return true;
}


bool Au_control_binding_iter_init_osc_depth_float(
        Au_control_binding_iter* iter,
        const Au_control_vars* aucv,
        const char* var_name,
        double depth)
{
    assert(iter != NULL);
    assert(aucv != NULL);
    assert(var_name != NULL);
    assert(isfinite(depth));

    iter->iter = NULL;
    iter->src_value.type = VALUE_TYPE_FLOAT;
    iter->src_value.value.float_type = depth;

    if (!Au_control_binding_iter_init_common(
                iter, aucv, var_name, ITER_MODE_OSC_DEPTH_FLOAT))
        return false;

    const Var_entry* var_entry = AAtree_get_exact(aucv->vars, var_name);
    assert(var_entry != NULL);

    const double min_value = var_entry->ext.float_slide_type.min_value;
    const double max_value = var_entry->ext.float_slide_type.max_value;
    const double src_range = max_value - min_value;
    if (src_range > range_eps)
    {
        const double range_norm_unclamped = depth / src_range;
        iter->ext.osc_float_type.osc_range_norm = clamp(range_norm_unclamped, -1.0, 1.0);
    }
    else
    {
        iter->ext.osc_float_type.osc_range_norm = 0.0;
    }

    Au_control_binding_iter_update(iter);

    return true;
}


bool Au_control_binding_iter_get_next_entry(Au_control_binding_iter* iter)
{
    assert(iter != NULL);
    assert(iter->iter != NULL);

    iter->iter = iter->iter->next;

    if (iter->iter == NULL)
        return false;

    Au_control_binding_iter_update(iter);

    return true;
}


static const char* mem_error_str =
    "Could not allocate memory for audio unit control variables";


static Bind_entry* new_Bind_entry_common(Streader* sr)
{
    assert(sr != NULL);

    char target_dev_name[16] = "";
    char target_var_name[KQT_VAR_NAME_MAX + 1] = "";

    if (!Streader_readf(
                sr,
                "%s,%s,",
                16,
                target_dev_name,
                KQT_VAR_NAME_MAX,
                target_var_name))
        return NULL;

    // Parse target device name
    Target_dev_type target_dev_type = TARGET_DEV_NONE;
    int target_dev_index = -1;
    if (string_has_prefix(target_dev_name, "au_"))
    {
        target_dev_type = TARGET_DEV_AU;
        target_dev_index = string_extract_index(target_dev_name, "au_", 2, "");
    }
    else if (string_has_prefix(target_dev_name, "proc_"))
    {
        target_dev_type = TARGET_DEV_PROC;
        target_dev_index = string_extract_index(target_dev_name, "proc_", 2, "");
    }

    if ((target_dev_type == TARGET_DEV_NONE) || (target_dev_index < 0))
    {
        Streader_set_error(sr, "Invalid target device name: %s", target_dev_name);
        return NULL;
    }

    // Check target variable path
    if (!is_valid_var_path(target_var_name))
    {
        Streader_set_error(
                sr,
                "Illegal target variable path %s"
                    " (Variable path components must contain"
                    " only lower-case letters and underscores"
                    " (and digits as other than first characters))",
                target_var_name);
        return NULL;
    }

    Bind_entry* bind_entry = memory_alloc_item(Bind_entry);
    if (bind_entry == NULL)
    {
        Streader_set_memory_error(sr, mem_error_str);
        return NULL;
    }

    // Initialise common fields
    bind_entry->type = BIND_ENTRY_TYPE_INVALID;
    bind_entry->target_dev_type = target_dev_type;
    bind_entry->target_dev_index = target_dev_index;
    bind_entry->target_var_type = VALUE_TYPE_NONE;
    strcpy(bind_entry->target_var_name, target_var_name);
    bind_entry->next = NULL;

    return bind_entry;
}


static bool read_binding_targets_generic(Streader* sr, int32_t index, void* userdata)
{
    assert(sr != NULL);
    ignore(index);

    Var_entry* entry = userdata;
    assert(entry != NULL);

    if (!Streader_readf(sr, "["))
        return false;

    // Create and attach new bind entry
    Bind_entry* bind_entry = new_Bind_entry_common(sr);
    if (bind_entry == NULL)
        return false;

    if (entry->last_bind_entry == NULL)
    {
        assert(entry->first_bind_entry == NULL);
        entry->first_bind_entry = bind_entry;
        entry->last_bind_entry = bind_entry;
    }
    else
    {
        entry->last_bind_entry->next = bind_entry;
        entry->last_bind_entry = bind_entry;
    }

    // Get target variable type
    char type_name[16] = "";
    if (!Streader_readf(sr, "%s,", 16, type_name))
        return false;

    if (string_eq(type_name, "bool"))
        bind_entry->target_var_type = VALUE_TYPE_BOOL;
    else if (string_eq(type_name, "int"))
        bind_entry->target_var_type = VALUE_TYPE_INT;
    else if (string_eq(type_name, "float"))
        bind_entry->target_var_type = VALUE_TYPE_FLOAT;
    else if (string_eq(type_name, "tstamp"))
        bind_entry->target_var_type = VALUE_TYPE_TSTAMP;

    if (bind_entry->target_var_type == VALUE_TYPE_NONE)
    {
        Streader_set_error(sr, "Invalid target variable type: %s", type_name);
        return false;
    }

    bind_entry->type = BIND_ENTRY_TYPE_EXPRESSION;
    bind_entry->ext.expr_type.expression = NULL;

    // Get memory area of the expression string
    Streader_skip_whitespace(sr);
    const char* const expr = Streader_get_remaining_data(sr);
    if (!Streader_read_string(sr, 0, NULL))
        return false;

    const char* const expr_end = Streader_get_remaining_data(sr);

    if (!Streader_readf(sr, "]"))
        return false;

    // Allocate space for the expression string
    assert(expr_end != NULL);
    assert(expr_end > expr);
    const int expr_length = expr_end - expr;

    bind_entry->ext.expr_type.expression = memory_calloc_items(char, expr_length + 1);
    if (bind_entry->ext.expr_type.expression == NULL)
    {
        Streader_set_memory_error(
                sr, "Could not allocate memory for audio unit control variables");
        return false;
    }

    strncpy(bind_entry->ext.expr_type.expression, expr, expr_length);
    bind_entry->ext.expr_type.expression[expr_length] = '\0';

    return true;
}


static bool read_binding_targets_float_slide(Streader* sr, int32_t index, void* userdata)
{
    assert(sr != NULL);
    ignore(index);

    Var_entry* entry = userdata;
    assert(entry != NULL);

    if (!Streader_readf(sr, "["))
        return false;

    // Create and attach new bind entry
    Bind_entry* bind_entry = new_Bind_entry_common(sr);
    if (bind_entry == NULL)
        return false;

    if (entry->last_bind_entry == NULL)
    {
        assert(entry->first_bind_entry == NULL);
        entry->first_bind_entry = bind_entry;
        entry->last_bind_entry = bind_entry;
    }
    else
    {
        entry->last_bind_entry->next = bind_entry;
        entry->last_bind_entry = bind_entry;
    }

    // Get type
    char type_name[16] = "";
    if (!Streader_readf(sr, "%s", 16, type_name))
        return false;

    // Read type-specific parts
    bind_entry->type = BIND_ENTRY_TYPE_FLOAT_SLIDE;
    if (string_eq(type_name, "float"))
    {
        if (!Streader_readf(
                    sr,
                    ",%f,%f]",
                    &bind_entry->ext.float_slide_type.map_min_to,
                    &bind_entry->ext.float_slide_type.map_max_to))
            return false;

        bind_entry->target_var_type = VALUE_TYPE_FLOAT;
    }
    else
    {
        Streader_set_error(
                sr,
                "Invalid type of audio unit control variable target %s: %s"
                " (Floating-point values with sliding/oscillating support"
                " may only be bound to other floating-point values)",
                bind_entry->target_var_name,
                type_name);
        return false;
    }

    return true;
}


static bool read_ext_data_float(Streader* sr, int32_t index, void* userdata)
{
    assert(sr != NULL);

    Var_entry* entry = userdata;
    assert(entry != NULL);

    if (index == 0)
    {
        if (!Streader_read_float(sr, &entry->ext.float_slide_type.min_value))
            return false;
    }
    else if (index == 1)
    {
        if (!Streader_read_float(sr, &entry->ext.float_slide_type.max_value))
            return false;
    }
    else
    {
        Streader_set_error(
                sr,
                "Unexpected data in type-specific parameter list of variable %s",
                entry->name);
        return false;
    }

    return true;
}


static bool read_var_entry(Streader* sr, int32_t index, void* userdata)
{
    assert(sr != NULL);
    ignore(index);

    Au_control_vars* acv = userdata;
    assert(acv != NULL);

    // Read type and name
    char type_name[16] = "";
    char var_name[KQT_VAR_NAME_MAX + 1] = "";

    if (!Streader_readf(sr, "[%s,%s,", 16, type_name, KQT_VAR_NAME_MAX + 1, var_name))
        return false;

    if (!is_valid_var_name(var_name))
    {
        Streader_set_error(
                sr,
                "Illegal variable name %s"
                    " (Variable names may only contain"
                    " lower-case letters and underscores"
                    " (and digits as other than first characters))",
                var_name);
        return false;
    }

    // Create and add new variable entry
    Var_entry* entry = memory_alloc_item(Var_entry);
    if (entry == NULL)
    {
        Streader_set_memory_error(sr, mem_error_str);
        return false;
    }

    strcpy(entry->name, var_name);
    entry->type = VAR_ENTRY_INVALID;
    entry->init_value = *VALUE_AUTO;
    entry->first_bind_entry = NULL;
    entry->last_bind_entry = NULL;

    if (!AAtree_ins(acv->vars, entry))
    {
        memory_free(entry);
        Streader_set_memory_error(sr, mem_error_str);
        return false;
    }

    // Read type-specific parts
    if (string_eq(type_name, "bool"))
    {
        entry->type = VAR_ENTRY_BOOL;

        entry->init_value.type = VALUE_TYPE_BOOL;

        if (!Streader_readf(
                    sr,
                    "%b,[],%l]",
                    &entry->init_value.value.bool_type,
                    read_binding_targets_generic,
                    entry))
            return false;
    }
    else if (string_eq(type_name, "int"))
    {
        entry->type = VAR_ENTRY_INT;

        entry->init_value.type = VALUE_TYPE_INT;

        if (!Streader_readf(
                    sr,
                    "%i,[],%l]",
                    &entry->init_value.value.int_type,
                    read_binding_targets_generic,
                    entry))
            return false;
    }
    else if (string_eq(type_name, "float"))
    {
        entry->type = VAR_ENTRY_FLOAT;

        entry->init_value.type = VALUE_TYPE_FLOAT;

        if (!Streader_readf(
                    sr,
                    "%f,[],%l]",
                    &entry->init_value.value.float_type,
                    read_binding_targets_generic,
                    entry))
            return false;
    }
    else if (string_eq(type_name, "tstamp"))
    {
        entry->type = VAR_ENTRY_TSTAMP;

        entry->init_value.type = VALUE_TYPE_TSTAMP;

        if (!Streader_readf(
                    sr,
                    "%t,[],%l]",
                    &entry->init_value.value.Tstamp_type,
                    read_binding_targets_generic,
                    entry))
            return false;
    }
    else if (string_eq(type_name, "float_slide"))
    {
        entry->type = VAR_ENTRY_FLOAT_SLIDE;
        entry->ext.float_slide_type.min_value = NAN;
        entry->ext.float_slide_type.max_value = NAN;

        entry->init_value.type = VALUE_TYPE_FLOAT;

        if (!Streader_readf(
                    sr,
                    "%f,%l,%l]",
                    &entry->init_value.value.float_type,
                    read_ext_data_float,
                    entry,
                    read_binding_targets_float_slide,
                    entry))
            return false;

        if (!isfinite(entry->ext.float_slide_type.min_value) ||
                !isfinite(entry->ext.float_slide_type.max_value))
        {
            Streader_set_error(
                    sr,
                    "Missing complete bounds information for floating-point variable %s",
                    entry->name);
            return false;
        }
    }
    else
    {
        Streader_set_error(
                sr,
                "Invalid type of audio unit control variable %s: %s",
                var_name,
                type_name);
        return false;
    }

    return true;
}


Au_control_vars* new_Au_control_vars(Streader* sr)
{
    assert(sr != NULL);

    if (Streader_is_error_set(sr))
        return NULL;

    Au_control_vars* aucv = memory_alloc_item(Au_control_vars);
    if (aucv == NULL)
    {
        Streader_set_memory_error(sr, mem_error_str);
        return NULL;
    }

    aucv->vars = new_AAtree(
            (int (*)(const void*, const void*))strcmp,
            (void (*)(void*))del_Var_entry);
    if (aucv->vars == NULL)
    {
        del_Au_control_vars(aucv);
        Streader_set_memory_error(sr, mem_error_str);
        return NULL;
    }

    if (!Streader_read_list(sr, read_var_entry, aucv))
    {
        del_Au_control_vars(aucv);
        return NULL;
    }

    return aucv;
}


void del_Au_control_vars(Au_control_vars* aucv)
{
    if (aucv == NULL)
        return;

    del_AAtree(aucv->vars);
    memory_free(aucv);

    return;
}


