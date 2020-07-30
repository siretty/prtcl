#!/usr/bin/env python3

import ctypes
import numpy as np
import matplotlib.pyplot as plt


def _load_size(file):
    size = ctypes.c_size_t()
    file.readinto(size)
    return size.value

def _load_string(file):
    len = _load_size(file)
    buf = file.read(len)

    return buf.decode('UTF-8')

def _load_ctype(file):
    name = _load_string(file)
    return {
        'b': np.dtype('b'),
        's32': np.dtype('i4'),
        's64': np.dtype('i8'),
        'f32': np.dtype('float32'),
        'f64': np.dtype('float64'),
    }[name]

def _load_shape(file):
    extents = []

    rank = _load_size(file)

    for dim in range(rank):
        extent = _load_size(file)
        extents.append(extent)

    return extents


def _load_uniform_field(file):
    name = _load_string(file)
    ctype = _load_ctype(file)
    shape = _load_shape(file)
    count = np.prod(shape, initial=1, dtype='i')
    array = np.fromfile(file, dtype=ctype, count=count)
    array = np.reshape(array, shape)

    return name, {'ctype': ctype, 'item_shape': shape, 'data': array}

def _load_varying_field(file):
    name = _load_string(file)
    ctype = _load_ctype(file)
    shape = _load_shape(file)
    item_count = _load_size(file)
    total_count = item_count * np.prod(shape, initial=1, dtype='i')
    array = np.fromfile(file, dtype=ctype, count=total_count)
    array = np.reshape(array, [item_count, *shape])

    return name, {'ctype': ctype, 'item_shape': shape, 'data': array}


def _load_group(file):
    name = _load_string(file)
    type = _load_string(file)

    tag_count = _load_size(file)
    tags = [_load_string(file) for tag_index in range(tag_count)]

    varying_fields = {}
    uniform_fields = {}

    particle_count = _load_size(file)

    vf_count = _load_size(file)
    for field_index in range(vf_count):
        vf_name, vf_data = _load_varying_field(file)
        varying_fields[vf_name] = vf_data

        # print(vf_name, vf_data['ctype'], vf_data['item_shape'], vf_data['data'])

    uf_count = _load_size(file)
    for uf_index in range(uf_count):
        uf_name, uf_data = _load_uniform_field(file)
        uniform_fields[uf_name] = uf_data

        # print(uf_name, uf_data['ctype'], uf_data['item_shape'], uf_data['data'])

    return name, {'type': type, 'tags': tags, 'varying_fields': varying_fields, 'uniform_fields': uniform_fields}


def load_model(file):
    model = {
        'global_fields': {},
        'groups': {},
    }

    count_gf = _load_size(file)
    for index_gf in range(count_gf):
        gf_name, gf_data = _load_uniform_field(file)
        model['global_fields'][gf_name] = gf_data

        # print(gf_name, gf_data['ctype'], gf_data['item_shape'], gf_data['data'])

    group_count = _load_size(file)
    for group_index in range(group_count):
        group_name, group_data = _load_group(file)
        model['groups'][group_name] = group_data

        # print(group_name, group_data)

    return model

def compute_total_momentum(data):
    speed = np.linalg.norm(data['varying_fields']['velocity']['data'], axis=1)
    mass = data['varying_fields']['mass']['data']
    return (speed * mass).sum()

def plot_total_momentum(model_file_names, group_name):
    p = []
    for model_file_name in model_file_names:
        with open(model_file_name, 'rb') as f:
            model = load_model(f)
        p.append(compute_total_momentum(model['groups'][group_name]))
    p = np.asarray(p)

    fig = plt.figure(figsize=(10, 3))
    axs = np.asarray(fig.subplots(1, 1)).flatten()
    axs[0].plot(np.arange(len(p)), p)
    axs[0].set_ylim(p.min() * 0.9, p.max() * 1.1)
    fig.show()
