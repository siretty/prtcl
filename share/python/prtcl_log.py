#!/usr/bin/env python3

import re

import numpy as np

def parse_log(file, last_step='integrate_position'):
    beg_re = re.compile(r"\[([0-9]+)[|][^]]*].*RunProcedure\('([^']*)'\) \[\[\[")
    pcg_re = re.compile(r"\[([0-9]+)[|][^]]*] pcg solver iterations ([0-9]+)")
    end_re = re.compile(r"\[([0-9]+)[|][^]]*].*RunProcedure\('([^']*)'\) ]]]")

    parts = []
    steps = []

    while True:
        line = file.readline()

        m = beg_re.search(line)
        if m is not None:
            steps.append([m.group(2), int(m.group(1)), None, None, []])
            continue

        m = pcg_re.search(line)
        if m is not None:
            steps[-1][4].append(int(m.group(2)))

        m = end_re.search(line)
        if m is not None:
            if steps[-1][0] == m.group(2):
                steps[-1][2] = int(m.group(1))
                steps[-1][3] = (steps[-1][2] - steps[-1][1]) / 1.
                l0e9

                if steps[-1][0] == last_step:
                    parts.append(steps)
                    steps = []
            else:
                raise Warning("ending step that hasn't begun")
            continue

        if 0 == len(line):
            break

    return parts

def stackplot_parts(parts):
    import matplotlib.pyplot as plt

    fig = plt.figure()
    axs = np.asarray(fig.subplots(2, 1)).flatten()

    # s: the step index
    s = np.arange(len(parts))

    # t: the time in (ns [I think]) each procedure took
    t = np.asarray(list(map(lambda part: tuple(step[3] for step in part), parts)))

    axs[0].stackplot(s, t.T, labels=[step[0] for step in parts[0]])
    axs[0].legend()

    # n: the number of iterations of the solvers
    n = np.asarray(list(map(lambda part: tuple(sum(step[4]) for step in part if 0 != len(step[4])), parts)))

    axs[1].stackplot(s, n.T, labels=[step[0] for step in parts[0] if 0 != len(step[4])])
    axs[1].legend()

    fig.show()
