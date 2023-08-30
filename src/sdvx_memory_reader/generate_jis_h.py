import numpy as np

jis = np.zeros((6879,), np.uint16)
unicode = np.zeros((6879,), np.uint16)
f = open("JIS0208.TXT", "r")

while (line := f.readline().strip()).startswith("#"): pass
for i in range(jis.shape[0]):
    h1, _, h2, _ = line.split("\t")
    jis[i], unicode[i] = int(h1[2:], 16), int(h2[2:], 16)
    line = f.readline().strip()
f.close()


with open("src/jis.h", "w") as f:
    f.write("#ifndef JIS_H\n#define JIS_H\n\n#include \"wchar.h\"\n\n")

    groups = [[]]
    last_num = jis[0]
    for i in range(1, len(jis)):
        if jis[i] != last_num+1:
            groups.append([])
        last_num = jis[i]
        groups[-1].append((jis[i], unicode[i]))

    f.write("typedef struct _DECODE_PAIR {\n"
            "\tconst wchar_t jis;\n"
            "\tconst wchar_t unicode;\n"
            "} DECODE_PAIR;\n\n"
            "typedef struct _DECODE_GROUP {\n"
            "\tconst DECODE_PAIR* pairs;\n"
            "\tconst unsigned char numPairs;\n"
            "\tconst wchar_t minVal;\n"
            "\tconst wchar_t maxVal;\n"
            "} DECODE_GROUP;\n\n"
            "typedef struct _DECODE_GROUPS {\n"
            f"\tconst DECODE_GROUP* groups[{len(groups)}];\n"
            f"\tconst unsigned char numGroups;\n"
            "} DECODE_GROUPS;\n\n")

    for i, group in enumerate(groups):
        f.write(f"const DECODE_PAIR _PAIRS{i}[{len(group)}] = "+"{\n\t")
        f.write(",\n\t".join(map(lambda item: "{%s, %s}" %(hex(item[0]), hex(item[1])), group)))
        f.write("\n};\n")
        f.write("const DECODE_GROUP _GROUP%d = {_PAIRS%d, %d, %s, %s};\n\n" %(i, i, len(group), hex(group[0][0]), hex(group[-1][0])))
    
    f.write("const DECODE_GROUPS DECODING = {{\n\t%s\n}, %d};\n\n" %(",\n\t".join(map(lambda n: f"&_GROUP{n}", range(len(groups)))), len(groups)))
    f.write("#endif /* JIS_H */")
