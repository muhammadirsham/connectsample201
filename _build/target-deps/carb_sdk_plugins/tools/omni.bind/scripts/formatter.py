__copyright__ = "Copyright (c) 2020-2022, NVIDIA CORPORATION. All rights reserved."
__license__ = """
NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
"""

from omni.repo.format import format_single_cpp


def format_cpp(path, real_file_name, repo_root):
    # we run format twice since clang-format seems to take two tries to stabilize its output
    for i in range(2):
        format_single_cpp(path, real_file_name, repo_root=repo_root)
    return True
