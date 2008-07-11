# Copyright (c) 2006 The Regents of The University of Michigan
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: Nathan Binkert

__all__ = [ 'attrdict', 'optiondict' ]

class attrdict(dict):
    def __getattr__(self, attr):
        if attr in self:
            return self.__getitem__(attr)
        return super(attrdict, self).__getattribute__(attr)

    def __setattr__(self, attr, value):
        if attr in dir(self):
            return super(attrdict, self).__setattr__(attr, value)
        return self.__setitem__(attr, value)

    def __delattr__(self, attr):
        if attr in self:
            return self.__delitem__(attr)
        return super(attrdict, self).__delattr__(attr, value)

class optiondict(attrdict):
    def __getattr__(self, attr):
        try:
            return super(optiondict, self).__getattr__(attr)
        except AttributeError:
            #d = optionsdict()
            #setattr(self, attr, d)
            return None

if __name__ == '__main__':
    x = attrdict()
    x.y = 1
    x['z'] = 2
    print x['y'], x.y
    print x['z'], x.z
    print dir(x)
    print x

    print

    del x['y']
    del x.z
    print dir(x)
    print(x)