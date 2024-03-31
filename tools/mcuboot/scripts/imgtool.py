#! /usr/bin/env python3
#
# Copyright 2017 Linaro Limited
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

use_virtual_env=True
#use_virtual_env=False
if use_virtual_env==True:
    from imgtool import main
else:
    from imgtool import main_opt

if __name__ == '__main__':
    if use_virtual_env==True:
        main.imgtool()
    else:
        main_opt.imgtool()
