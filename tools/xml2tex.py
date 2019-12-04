#!/usr/bin/env python3

import xml.sax, sys

class XmlToTexHandler(xml.sax.ContentHandler):
    def __init__(self):
        self.__tags = []
        self.__attr = []
        self.pieces = []

    # tag: section

    def otag_section(self, attributes):
        raw = r"\begin{PrtclSection}"
        if "name" in attributes:
            raw += "{" + attributes["name"] + "}"
        self.pieces.append('')
        self.pieces.append(raw)

    def ctag_section(self):
        self.pieces.append(r"\end{PrtclSection}")
        self.pieces.append('')

    # tag: particle_loop

    def otag_particle_loop(self, attributes):
        self.pieces.append(r"\begin{PrtclForeachParticle}")

    def ctag_particle_loop(self):
        self.pieces.append(r"\end{PrtclForeachParticle}")

    # tag: neighbour_loop

    def otag_neighbour_loop(self, attributes):
        self.pieces.append(r"\begin{PrtclForeachNeighbour}")

    def ctag_neighbour_loop(self):
        self.pieces.append(r"\end{PrtclForeachNeighbour}")

    # tag: selector

    def otag_selector(self, attributes):
        self.pieces.append(r"\begin{PrtclSelector}")

    def ctag_selector(self):
        self.pieces.append(r"\end{PrtclSelector}")

    # tag: eq

    def otag_eq(self, attributes):
        self.pieces.append(r"\PrtclEq{")

    def ctag_eq(self):
        self.pieces.append(r"}")

    # tag: rd

    def otag_rd(self, attributes):
        self.pieces.append(r"\PrtclRd{")

    def ctag_rd(self):
        self.pieces.append(r"}")

    # tag: field

    def otag_field(self, attributes):
        self.omath()
        raw = r"\PrtclField"
        raw += attributes['kind'].capitalize()
        raw += attributes['type'].capitalize()
        self.pieces.append(raw + '{')

    def text_field(self, text):
        text = text.strip()
        alias = {
            'position': 'x',
            'velocity': 'v',
            'acceleration': 'a',
            'density': r'\rho',
            'pressure': 'p',
            'time_step': r'\Delta t',
            'viscosity': r'\nu',
            'mass': 'm',
            'perfect_sampling_distance': 'h',
            'rest_density': r'\rho^0',
            'volume': 'V',
            'compressibility': r'\kappa',
            'gravity': 'g',
        }.get(text, None)
        if alias is not None:
            self.pieces.append(alias)
        else:
            attr = self.__attr[-1]
            text = text.replace('_', r'\_')
            self.pieces.append(r'\raisebox{0.002\hsize}{\resizebox{0.06\hsize}{!}{[' + attr['kind'].lower()[0] + attr['type'].lower()[0] + ':' + text + r']}}')

    def ctag_field(self):
        self.pieces.append(r"}")
        self.cmath()

    # tag: subscript

    def otag_subscript(self, attributes):
        self.omath()
        self.pieces.append(r"{")

    def ctag_subscript(self):
        self.pieces.append(r"}")
        self.cmath()

    # tag: group

    def otag_group(self, attributes):
        self.omath()

    def text_group(self, text):
        text = text.strip()
        self.pieces.append({
            'active': 'i',
            'passive': 'j',
        }.get(text, text))

    def ctag_group(self):
        self.cmath()

    # tag: binary

    def otag_binary(self, attributes):
        self.omath()
        self.pieces.append('{')
        if self.__tags[-2] == 'binary':
            current_op = self.__attr[-1]['op']
            parent_op = self.__attr[-2]['op']
            if current_op != '/' and parent_op != '/' and parent_op != current_op:
                self.pieces.append(r'\left(')

    def ctag_binary(self):
        if self.__tags[-2] == 'binary':
            current_op = self.__attr[-1]['op']
            parent_op = self.__attr[-2]['op']
            if current_op != '/' and parent_op != '/' and parent_op != current_op:
                self.pieces.append(r'\right)')
        self.pieces.append('}')
        self.cmath()
    
    # tag: value

    def otag_value(self, attributes):
        self.omath()

    def text_value(self, text):
        self.pieces.append(text)

    def ctag_value(self):
        self.cmath()

    # tag: call

    def otag_call(self, attributes):
        self.omath()
        name = attributes['name'].strip()
        if name in ['norm', 'norm_squared']:
            self.pieces.append(r'\left\lVert')
        elif name == 'dot':
            self.pieces.append(r'\left\langle')
        else:
            name = {
                'kernel': 'W',
                'kernel_gradient': r'\nabla W',
            }.get(name, name)
            self.pieces.append(r'\operatorname{' + name + r'}\mkern-3mu\left(')

    def ctag_call(self):
        name = self.__attr[-1]['name'].strip()
        if name == 'norm':
            self.pieces.append(r'\right\rVert')
        elif name == 'norm_squared':
            self.pieces.append(r'\right\rVert^2')
        elif name == 'dot':
            self.pieces.append(r'\right\rangle')
        else:
            self.pieces.append(r'\right)')
        self.cmath()

    # helper functions for mathematical operations

    def omath(self):
        parent_tag = self.__tags[-2]
        parent_child_counter = self.__attr[-2]['xml2tex@child_counter']
        if parent_tag == 'assign':
            if parent_child_counter == 2:
                self.pieces.append(r'\,=\,')
        if parent_tag == 'opassign':
            if parent_child_counter == 2:
                op = self.__attr[-2]['op']
                self.pieces.append(r'\,\mathop{' + op[0] + r'\mkern-5mu' + op[1] + r'}\,')
        if parent_tag == 'binary':
            op = self.__attr[-2]['op'].strip()
            if parent_child_counter == 1:
                if op == '/':
                    self.pieces.append(r'\frac{')
            if parent_child_counter == 2:
                if op in ['+', '-']:
                    self.pieces.append(self.__attr[-2]['op'])
                if op == '/':
                    self.pieces.append('{')
                if op == '*':
                    self.pieces.append(r'\,') # insert small space
                    # self.pieces.append(r'\cdot') # alternative
        if parent_tag == 'call':
            if parent_child_counter > 1:
                self.pieces.append(' , ')
        if parent_tag == 'subscript' and self.__attr[-2]['xml2tex@child_counter'] == 2:
            self.pieces.append(r"_{")

    def cmath(self):
        parent_child_counter = self.__attr[-2]['xml2tex@child_counter']
        if self.__tags[-2] == 'subscript' and parent_child_counter == 2:
            self.pieces.append(r"}")
        if self.__tags[-2] == 'binary':
            op = self.__attr[-2]['op'].strip()
            if op == '/':
                self.pieces.append('}')

    # dispatch to per-tag members

    def noop_otag(self, attributes):
        pass

    def noop_ctag(self):
        pass

    def noop_text(self, text):
        pass

    def startElement(self, tag, attributes):
        if 0 < len(self.__attr):
            self.__attr[-1]['xml2tex@child_counter'] += 1
        self.__tags.append(tag)
        self.__attr.append(dict(attributes))
        self.__attr[-1]['xml2tex@child_counter'] = 0
        func = getattr(self, "otag_" + tag, self.noop_otag)
        func(self.__attr[-1])

    def endElement(self, tag):
        func = getattr(self, "ctag_" + tag, self.noop_ctag)
        func()
        if tag != self.__tags.pop():
            raise RuntimeError("invalid tag on stack")
        self.__attr.pop()

    def characters(self, content):
        func = getattr(self, "text_" + self.__tags[-1], self.noop_text)
        func(content)

_header = r"""
\documentclass{article}

\usepackage[a4paper,includeheadfoot,margin=2.54cm]{geometry}

\usepackage{amsmath}
\usepackage{algpseudocode}
\usepackage{graphicx}

% {{{ PrtclSection
\newenvironment{PrtclSection}[1]{%
\begin{algorithmic}%
\Procedure{#1}{}%
}{%
\EndProcedure%
\end{algorithmic}%
}
% }}}

% {{{ PrtclForeachParticle
\newenvironment{PrtclForeachParticle}{%
\ForAll{particles $i$}%
}{%
\EndFor%
}
% }}}

% {{{ PrtclForeachNeighbour
\newenvironment{PrtclForeachNeighbour}{%
\ForAll{neighbours $j$}%
}{%
\EndFor%
}
% }}}

% {{{ PrtclSelector
\newenvironment{PrtclSelector}{%
\If{...}%
}{%
\EndIf%
}
% }}}

% {{{ PrtclEq
\newcommand{\PrtclEq}[1]{\State{$ #1 $}}
% }}}

% {{{ PrtclRd
\newcommand{\PrtclRd}[1]{\State{#1}}
% }}}

% {{{ PrtclField...
\newcommand{\PrtclFieldVaryingScalar}[1]{#1}
\newcommand{\PrtclFieldUniformScalar}[1]{#1}
\newcommand{\PrtclFieldGlobalScalar}[1]{#1}

\newcommand{\PrtclFieldVaryingVector}[1]{\mathbf{#1}}
\newcommand{\PrtclFieldUniformVector}[1]{\mathbf{#1}}
\newcommand{\PrtclFieldGlobalVector}[1]{\mathbf{#1}}

\newcommand{\PrtclFieldVaryingMatrix}[1]{\mathbf{#1}}
\newcommand{\PrtclFieldUniformMatrix}[1]{\mathbf{#1}}
\newcommand{\PrtclFieldGlobalMatrix}[1]{\mathbf{#1}}
% }}}

\begin{document}
"""
_footer = r"""
\end{document}
"""

if __name__ == "__main__":
    # create the parser
    parser = xml.sax.make_parser()
    # disable namespaces
    parser.setFeature(xml.sax.handler.feature_namespaces, False)

    # create the content handler
    handler = XmlToTexHandler()

    # register the content handler
    parser.setContentHandler(handler)
    # parse from stdin
    parser.parse(sys.stdin)

    # print the result
    print(_header)
    print('\n'.join(handler.pieces))
    print(_footer)
    
