

/*
 * Author: Tomi Jylhä-Ollila, Finland 2010-2011
 *
 * This file is part of Kunquat.
 *
 * CC0 1.0 Universal, http://creativecommons.org/publicdomain/zero/1.0/
 *
 * To the extent possible under law, Kunquat Affirmers have waived all
 * copyright and related or neighboring rights to Kunquat.
 */


#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>
#include <math.h>

#include <AAtree.h>
#include <Connections.h>
#include <Connections_search.h>
#include <Device_node.h>
#include <DSP_table.h>
#include <Effect.h>
#include <Effect_table.h>
#include <string_common.h>
#include <xassert.h>
#include <xmemory.h>


struct Connections
{
    AAtree* nodes;
    AAiter* iter;
};


/**
 * Resets the graph for searching purposes.
 *
 * \param graph   The Connections -- must not be \c NULL.
 */
static void Connections_reset(Connections* graph);


/**
 * Tells whether there is a cycle inside Connections.
 *
 * All Connections must be acyclic.
 *
 * \param graph   The Connections -- must not be \c NULL.
 *
 * \return   \c true if there is a cycle in \a graph, otherwise \c false.
 */
static bool Connections_is_cyclic(Connections* graph);


/**
 * Validates a connection path.
 *
 * This function also strips the port directory off the path.
 *
 * \param str     The path -- must not be \c NULL.
 * \param level   The connection level -- must be valid.
 * \param type    The type of the path -- must be valid.
 * \param state   The Read state -- must not be \c NULL.
 *
 * \return   The port number if the path is valid, otherwise \c -1.
 */
static int validate_connection_path(char* str,
                                    Connection_level level,
                                    Device_port_type type,
                                    Read_state* state);


#define clean_if(expr, graph, node)    \
    if (true)                          \
    {                                  \
        if ((expr))                    \
        {                              \
            if (node != NULL)          \
            {                          \
                del_Device_node(node); \
            }                          \
            del_Connections(graph);    \
            return NULL;               \
        }                              \
    } else (void)0

Connections* new_Connections_from_string(char* str,
                                         Connection_level level,
                                         Ins_table* insts,
                                         Effect_table* effects,
                                         DSP_table* dsps,
                                         Device* master,
                                         Read_state* state)
{
    assert((level & ~(CONNECTION_LEVEL_INSTRUMENT |
                     CONNECTION_LEVEL_EFFECT)) == 0);
    assert(insts != NULL);
    assert(effects != NULL);
    assert(!(level & CONNECTION_LEVEL_EFFECT) || (dsps != NULL));
    assert((dsps == NULL) || (level & CONNECTION_LEVEL_EFFECT));
    assert(master != NULL);
    assert(state != NULL);
    if (state->error)
    {
        return NULL;
    }
    Connections* graph = xalloc(Connections);
    if (graph == NULL)
    {
        return NULL;
    }
    graph->nodes = NULL;
    graph->iter = NULL;
    graph->nodes = new_AAtree((int (*)(const void*, const void*))Device_node_cmp,
                                 (void (*)(void*))del_Device_node);
    clean_if(graph->nodes == NULL, graph, NULL);
    graph->iter = new_AAiter(graph->nodes);
    clean_if(graph->iter == NULL, graph, NULL);

    Device_node* master_node = NULL;
    if ((level & CONNECTION_LEVEL_EFFECT))
    {
        Device* iface = Effect_get_output_interface((Effect*)master);
        master_node = new_Device_node("", insts, effects, dsps, iface);
    }
    else
    {
        master_node = new_Device_node("", insts, effects, dsps, master);
    }
    clean_if(master_node == NULL, graph, NULL);
    clean_if(!AAtree_ins(graph->nodes, master_node), graph, master_node);

    if (str == NULL)
    {
        Connections_reset(graph);
        return graph;
    }

    str = read_const_char(str, '[', state);
    clean_if(state->error, graph, NULL);
    str = read_const_char(str, ']', state);
    if (!state->error)
    {
        Connections_reset(graph);
        return graph;
    }
    Read_state_clear_error(state);

    bool expect_entry = true;
    while (expect_entry)
    {
        str = read_const_char(str, '[', state);
        char src_name[KQT_DEVICE_NODE_NAME_MAX] = { '\0' };
        str = read_string(str, src_name, KQT_DEVICE_NODE_NAME_MAX, state);
        str = read_const_char(str, ',', state);
        char dest_name[KQT_DEVICE_NODE_NAME_MAX] = { '\0' };
        str = read_string(str, dest_name, KQT_DEVICE_NODE_NAME_MAX, state);
        str = read_const_char(str, ']', state);

        int src_port = validate_connection_path(src_name,
                                                level,
                                                DEVICE_PORT_TYPE_SEND,
                                                state);
        int dest_port = validate_connection_path(dest_name,
                                                 level,
                                                 DEVICE_PORT_TYPE_RECEIVE,
                                                 state);
        clean_if(state->error, graph, NULL);

        if ((level & CONNECTION_LEVEL_EFFECT))
        {
            if (string_eq(src_name, ""))
            {
                strcpy(src_name, "Iin");
            }
        }

        if (AAtree_get_exact(graph->nodes, src_name) == NULL)
        {
            Device* actual_master = master;
            if ((level & CONNECTION_LEVEL_EFFECT) &&
                    string_eq(src_name, "Iin"))
            {
                actual_master = Effect_get_input_interface((Effect*)master);
            }
            Device_node* new_src = new_Device_node(src_name,
                                                   insts, effects, dsps,
                                                   actual_master);
            clean_if(new_src == NULL, graph, NULL);
            clean_if(!AAtree_ins(graph->nodes, new_src), graph, new_src);
        }
        Device_node* src_node = AAtree_get_exact(graph->nodes, src_name);

        if (AAtree_get_exact(graph->nodes, dest_name) == NULL)
        {
#if 0
            Device* actual_master = master;
            if ((level & CONNECTION_LEVEL_EFFECT) &&
                    string_eq(dest_name, ""))
            {
                actual_master = Effect_get_output_interface((Effect*)master);
            }
#endif
            Device_node* new_dest = new_Device_node(dest_name,
                                                    insts, effects, dsps,
                                                    master);
            clean_if(new_dest == NULL, graph, NULL);
            clean_if(!AAtree_ins(graph->nodes, new_dest), graph, new_dest);
        }
        Device_node* dest_node = AAtree_get_exact(graph->nodes, dest_name);

        assert(src_node != NULL);
        assert(dest_node != NULL);
        clean_if(!Device_node_connect(dest_node, dest_port,
                                     src_node, src_port), graph, NULL);

        check_next(str, state, expect_entry);
    }
    str = read_const_char(str, ']', state);
    clean_if(state->error, graph, NULL);

    if (Connections_is_cyclic(graph))
    {
        Read_state_set_error(state, "The connection graph contains a cycle");
        del_Connections(graph);
        return NULL;
    }
    Connections_reset(graph);
    return graph;
}

#undef clean_if


Device_node* Connections_get_master(Connections* graph)
{
    assert(graph != NULL);
    return AAtree_get_exact(graph->nodes, "");
}


bool Connections_prepare(Connections* graph)
{
    assert(graph != NULL);
    return Connections_init_buffers(graph);
}


bool Connections_init_buffers_simple(Connections* graph)
{
    assert(graph != NULL);
    Device_node* master = AAtree_get_exact(graph->nodes, "");
    assert(master != NULL);
    Device_node_reset(master);
    if (!Device_node_init_buffers_simple(master))
    {
        return false;
    }
    return Device_node_init_effect_buffers(master);
}


bool Connections_init_buffers(Connections* graph)
{
    assert(graph != NULL);
    Device_node* master = AAtree_get_exact(graph->nodes, "");
    assert(master != NULL);
//    fprintf(stderr, "\n\n!!! Entering Connections_init_buffers:\n");

    Device_node_reset(master);
    Device_node_remove_direct_buffers(master);
//    fprintf(stderr, "After Device_node_remove_direct_buffers:\n");
//    Connections_print(graph, stderr);

    Device_node_reset(master);
    if (!Device_node_init_input_buffers(master))
    {
        return false;
    }
//    fprintf(stderr, "After Device_node_init_input_buffers:\n");
//    Connections_print(graph, stderr);

    Device_node_reset(master);
    if (!Device_node_init_buffers_by_suggestion(master, 0, NULL))
    {
        return false;
    }
    Device_node_reset(master);
    if (!Device_node_init_effect_buffers(master))
    {
        return false;
    }
//    fprintf(stderr, "After Device_node_init_buffers_by_suggestion:\n");
//    Connections_print(graph, stderr);

 //   fprintf(stderr, "!!! Finished Connections_init_buffers\n\n");
    return true;
}


void Connections_clear_buffers(Connections* graph,
                               uint32_t start,
                               uint32_t until)
{
    assert(graph != NULL);
    Device_node* master = AAtree_get_exact(graph->nodes, "");
    assert(master != NULL);
    if (start >= until)
    {
        return;
    }
    Device_node_reset(master);
    Device_node_clear_buffers(master, start, until);
    return;
}


void Connections_mix(Connections* graph,
                     uint32_t start,
                     uint32_t until,
                     uint32_t freq,
                     double tempo)
{
    assert(graph != NULL);
    assert(freq > 0);
    assert(isfinite(tempo));
    assert(tempo > 0);
    Device_node* master = AAtree_get_exact(graph->nodes, "");
    assert(master != NULL);
    if (start >= until)
    {
        return;
    }
#if 0
    static bool called = false;
    if (!called)
    {
        Connections_print(graph, stderr);
    }
    called = true;
//    fprintf(stderr, "Mix process:\n");
#endif
    Device_node_reset(master);
    Device_node_mix(master, start, until, freq, tempo);
    return;
}


static void Connections_reset(Connections* graph)
{
    assert(graph != NULL);
    const char* name = "";
    Device_node* node = AAiter_get(graph->iter, name);
    while (node != NULL)
    {
        Device_node_set_state(node, DEVICE_NODE_STATE_NEW);
        node = AAiter_get_next(graph->iter);
    }
    return;
}


static bool Connections_is_cyclic(Connections* graph)
{
    assert(graph != NULL);
    Connections_reset(graph);
    const char* name = "";
    Device_node* node = AAiter_get(graph->iter, name);
    while (node != NULL)
    {
        assert(Device_node_get_state(node) != DEVICE_NODE_STATE_REACHED);
        if (Device_node_cycle_in_path(node))
        {
            return true;
        }
        node = AAiter_get_next(graph->iter);
    }
    return false;
}


void Connections_print(Connections* graph, FILE* out)
{
    assert(graph != NULL);
    assert(out != NULL);
//    Connections_reset(graph);
    Device_node* master = AAtree_get_exact(graph->nodes, "");
    assert(master != NULL);
    Device_node_print(master, out);
    fprintf(out, "\n");
    return;
}


void del_Connections(Connections* graph)
{
    if (graph == NULL)
    {
        return;
    }
    del_AAiter(graph->iter);
    del_AAtree(graph->nodes);
    xfree(graph);
    return;
}


static int read_index(char* str)
{
    assert(str != NULL);
    static const char* hex_digits = "0123456789abcdef";
    if (strspn(str, hex_digits) != 2)
    {
        return INT_MAX;
    }
    int res = (strchr(hex_digits, str[0]) - hex_digits) * 0x10;
    return res + (strchr(hex_digits, str[1]) - hex_digits);
}


static int validate_connection_path(char* str,
                                    Connection_level level,
                                    Device_port_type type,
                                    Read_state* state)
{
    assert(str != NULL);
    assert(type < DEVICE_PORT_TYPES);
    assert(state != NULL);
    if (state->error)
    {
        return -1;
    }
    bool instrument = false;
    bool generator = false;
    //bool effect = false;
    //bool dsp = false;
    bool root = true;
    char* path = str;
    char* trim_point = str;
    if (string_has_prefix(str, "ins_"))
    {
        if (level != CONNECTION_LEVEL_GLOBAL)
        {
            Read_state_set_error(state,
                    "Instrument directory in a deep-level connection:"
                    " \"%s\"", path);
            return -1;
        }
        instrument = true;
        root = false;
        str += strlen("ins_");
        if (read_index(str) >= KQT_INSTRUMENTS_MAX)
        {
            Read_state_set_error(state,
                    "Invalid instrument number in the connection:"
                    " \"%s\"", path);
            return -1;
        }
        str += 2;
        if (!string_has_prefix(str, "/" MAGIC_ID "iXX/"))
        {
            Read_state_set_error(state,
                    "Missing instrument header after the instrument number"
                    " in the connection: \"%s\"", path);
            return -1;
        }
        str += strlen("/" MAGIC_ID "iXX/");
        trim_point = str - 1;
    }
    else if (string_has_prefix(str, "eff_"))
    {
        if ((level & CONNECTION_LEVEL_EFFECT))
        {
            Read_state_set_error(state,
                    "Effect directory in an effect-level connection:"
                    " \"%s\"", path);
            return -1;
        }
        //effect = true;
        root = false;
        str += strlen("eff_");
        int max = KQT_EFFECTS_MAX;
        if ((level & CONNECTION_LEVEL_INSTRUMENT))
        {
            max = KQT_INST_EFFECTS_MAX;
        }
        if (read_index(str) >= max)
        {
            Read_state_set_error(state,
                    "Invalid effect number in the connection:"
                    " \"%s\"", path);
            return -1;
        }
        str += 2;
        if (!string_has_prefix(str, "/" MAGIC_ID "eXX/"))
        {
            Read_state_set_error(state,
                    "Missing effect header after the effect number in"
                    " the connection: \"%s\"", path);
            return -1;
        }
        str += strlen("/" MAGIC_ID "eXX/");
        trim_point = str - 1;
    }
    else if (string_has_prefix(str, "gen_"))
    {
        if (!(level & CONNECTION_LEVEL_INSTRUMENT))
        {
            Read_state_set_error(state,
                    "Generator directory in a root-level connection:"
                    " \"%s\"", path);
            return -1;
        }
        if ((level & CONNECTION_LEVEL_EFFECT))
        {
            Read_state_set_error(state,
                    "Generator directory in an effect-level connection:"
                    " \"%s\"", path);
            return -1;
        }
        root = false;
        generator = true;
        str += strlen("gen_");
        if (read_index(str) >= KQT_GENERATORS_MAX)
        {
            Read_state_set_error(state,
                    "Invalid generator number in the connection:"
                    " \"%s\"", path);
            return -1;
        }
        str += 2;
        if (!string_has_prefix(str, "/" MAGIC_ID "gXX/"))
        {
            Read_state_set_error(state,
                    "Missing generator header after the generator number"
                    " in the connection: \"%s\"", path);
            return -1;
        }
        str += strlen("/" MAGIC_ID "gXX/");
        if (!string_has_prefix(str, "C/"))
        {
            Read_state_set_error(state,
                    "Invalid generator parameter directory"
                    " in the connection: \"%s\"", path);
            return -1;
        }
        str += strlen("C/");
        trim_point = str - 1;
    }
    else if (string_has_prefix(str, "dsp_"))
    {
        if (!(level & CONNECTION_LEVEL_EFFECT))
        {
            Read_state_set_error(state,
                    "DSP directory outside an effect: \"%s\"", path);
            return -1;
        }
        root = false;
        //dsp = true;
        str += strlen("dsp_");
        if (read_index(str) >= KQT_DSPS_MAX)
        {
            Read_state_set_error(state,
                    "Invalid DSP number in the connection: \"%s\"", path);
            return -1;
        }
        str += 2;
        if (!string_has_prefix(str, "/" MAGIC_ID "dXX/"))
        {
            Read_state_set_error(state,
                    "Missing DSP header after the DSP number"
                    " in the connection: \"%s\"", path);
            return -1;
        }
        str += strlen("/" MAGIC_ID "dXX/");
        if (!string_has_prefix(str, "C/"))
        {
            Read_state_set_error(state,
                    "Invalid DSP parameter directory"
                    " in the connection: \"%s\"", path);
            return -1;
        }
        str += strlen("C/");
        trim_point = str - 1;
    }
    if (string_has_prefix(str, "in_") || string_has_prefix(str, "out_"))
    {
        // TODO: check effect connections
        if (string_has_prefix(str, "in_") && (instrument || generator))
        {
            Read_state_set_error(state,
                    "Input ports are not allowed for instruments"
                    " or generators: \"%s\"", path);
            return -1;
        }
        if (string_has_prefix(str, "in_") && root &&
                !(level & CONNECTION_LEVEL_EFFECT))
        {
            Read_state_set_error(state,
                    "Input ports are not allowed for master: \"%s\"", path);
            return -1;
        }
        if (type == DEVICE_PORT_TYPE_RECEIVE)
        {
            bool can_receive = (!root && string_has_prefix(str, "in_")) ||
                               (root && string_has_prefix(str, "out_"));
            if (!can_receive)
            {
                Read_state_set_error(state,
                        "Destination port is not for receiving data:"
                        " \"%s\"", path);
                return -1;
            }
        }
        else
        {
            assert(type == DEVICE_PORT_TYPE_SEND);
            bool can_send = (string_has_prefix(str, "out_") && !root) ||
                            (string_has_prefix(str, "in_") && root);
            if (!can_send)
            {
                Read_state_set_error(state,
                        "Source port is not for sending data: \"%s\"", path);
                return -1;
            }
        }
        str += strcspn(str, "_") + 1;
        int port = read_index(str);
        if (port >= KQT_DEVICE_PORTS_MAX)
        {
            Read_state_set_error(state, "Invalid port number: \"%s\"", path);
            return -1;
        }
        str += 2;
        if (str[0] != '/' && str[0] != '\0' && str[1] != '\0')
        {
            Read_state_set_error(state, "Connection path contains garbage"
                    " after the port specification: \"%s\"", path);
            return -1;
        }
        *trim_point = '\0';
        return port;
    }
    Read_state_set_error(state, "Invalid connection: \"%s\"", path);
    return -1;
}

