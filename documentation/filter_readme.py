import sys

if __name__ == "__main__":
    for line in sys.stdin:
        # Skip lines containing image markdown or HTML image tags
        if not ("![Actions Status]" in line or "![codecov]" in line or "<img src=" in line or "![Star History Chart]" in line):
            sys.stdout.write(line)