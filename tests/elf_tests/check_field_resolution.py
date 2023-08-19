import argparse
import subprocess
import sys


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--elf', help='Path to ELF file', required=True)
    parser.add_argument('--addr2field', help='Path to addr2field executable', required=True)
    parser.add_argument('--address', help='Address to check', required=True)
    parser.add_argument('--field', help='Expected', required=True)
    return parser.parse_args()


def main(args):
    cp = subprocess.run([args.addr2field, args.elf, args.address], stdout=subprocess.PIPE, encoding='utf-8')
    lines = cp.stdout.splitlines(keepends=False)
    resolutions = {}
    for line in lines:
        parts = line.split('-', maxsplit=1)
        if len(parts) != 2:
            continue

        resolutions[parts[0].strip().lower()] = parts[1].strip()

    if args.address.lower() not in resolutions:
        print('Address not resolved')
        return 1

    if resolutions[args.address.lower()] != args.field:
        print(f'Field resolution mismatch')
        print(f'Expected: {args.field}')
        print(f'Actual:   {resolutions[args.address.lower()]}')
        return 2

    print('Resolution OK')
    return 0


if __name__ == '__main__':
    sys.exit(main(parse_args()))
