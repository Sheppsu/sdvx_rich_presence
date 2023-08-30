import build
import subprocess


if __name__ == "__main__":
    try:
        exe = build.build()
        input("Press enter to run...")
        subprocess.run(["build/rich_presence.exe"])
    except KeyboardInterrupt:
        pass
