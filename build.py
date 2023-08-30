import subprocess
import os


library = [
    "src/sdvx_memory_reader/src/memory_reader.c",
    "src/sdvx_memory_reader/src/decode.c",
    "src/json.c",
    "src/discord.c",
    "src/rich_presence.c"
]


def build():
    o_files = []
    for file_path in library:
        file = os.path.split(file_path)[-1]
        o_file = "build/"+file[:-2]+".o"
        o_files.append(o_file)
        args = ["gcc", file_path, "-o", o_file, "-c"]
        print(" ".join(args))
        subprocess.run(args)
    args = ["gcc", "-o", f"build/rich_presence.exe"]+o_files+["-lShlwapi"]
    print(" ".join(args))
    subprocess.run(args)


if __name__ == "__main__":
    build()
