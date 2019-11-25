#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
import os, argparse, shutil, subprocess

def error(msg):
    print msg
    exit(1)

def is_windows():
    if os.name == 'nt':
        return True
    else:
        return False

def host_platform():
    if is_windows():
        return 'pc'
    else:
        return 'osx_gl'

def get_shader_list(game_name, shader_type, asset_platform, bin_folder, game_path, shader_list):
    """
    Gets the shader list for a specific platform using ShaderCacheGen.
    Right now the shader list will always output at Cache/<game_name>/<host_platform>/user/cache/Shaders/Cache/<platform>
    That will change when this is updated to take a destination path
    """
    shadergen_path = os.path.join(game_path, bin_folder, 'ShaderCacheGen')
    if is_windows():
        shadergen_path += '.exe'

    command_args = [
        shadergen_path,
        '/GetShaderList',
        '/ShadersPlatform={}'.format(shader_type)
    ]

    if not os.path.isfile(shadergen_path):
        error("[ERROR] ShaderCacheGen could not be found at {}".format(shadergen_path))
    else:
        command = ' '.join(command_args)
        print('[INFO] get_shader_list: Running command - {}'.format(command))
        try:
            subprocess.check_call(command, shell=True)
        except subprocess.CalledProcessError:
            error('[ERROR] Failed to get the shader list for {}'.format(shader_type))

parser = argparse.ArgumentParser(description='Gets the shader list for a specific platform from the current shader compiler server')
parser.add_argument('game_name', type=str, help="Name of the game")
parser.add_argument('asset_platform', type=str, help="Asset platform to use")
parser.add_argument('shader_type', type=str, help="The shader type to use")
parser.add_argument('bin_folder', type=str, help="Folder where the ShaderCacheGen executable lives. This is used along the game_path (game_path/bin_folder/ShaderCacheGen)")
parser.add_argument('-g', '--game_path', type=str, required=False, default=os.getcwd(), help="Path to the game root folder. This the same as engine_path for non external projects. Default is current directory.")
parser.add_argument('-s', '--shader_list', type=str, required=False, help="Optional path to the list of shaders. If not provided will use the list generated by the local shader compiler.")

args = parser.parse_args()

print '---- Getting shader list for {}----'.format(args.asset_platform)
get_shader_list(args.game_name, args.shader_type, args.asset_platform, args.bin_folder, args.game_path, args.shader_list)
print '---- Finish getting shader list -----'
