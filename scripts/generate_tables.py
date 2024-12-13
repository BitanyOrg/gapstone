import subprocess
import pathlib
import multiprocessing

TARGETS = {
    "tbl"
}

def tblgen(tblgen, llvm, file, output):
    cmd = f"{tblgen} {file} -I {file.parent} -I {llvm/'include'} --dump-json -o {output}"
    res = subprocess.run(cmd, shell=True)
    if res.returncode != 0:
        print(f"Failed to process {file}")
        
def find_targets(folder):
    folder_path = pathlib.Path(folder)
    td_files = folder_path.glob("*.td")
    pattern = 'include "llvm/Target/Target.td"'
    targets = []
    updated = True
    todos = list(td_files)
    while updated:
        updated = False
        for file in todos:
            content = file.open().read()
            if content.find(pattern) != -1:
                print(file.name)
                targets.append(file)
                todos.remove(file)
                updated = True
            else: 
                for target in targets:
                    target_pattern =  f'include "{target.name}"'
                    if content.find(target_pattern) != -1:
                        print(file.name)
                        targets.append(file)
                        todos.remove(file)
                        updated = True
    return targets

def main():
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--tblgen", help="Path to the tblgen executable")
    parser.add_argument("--llvm", help="Path to the llvm directory")
    parser.add_argument("--output", help="Path to the output directory")
    args = parser.parse_args()
    target_path = pathlib.Path(f"{args.llvm}/lib/Target")
    folders = [folder for folder in target_path.glob("*") if folder.is_dir()]
    tasks = []
    for folder in folders:
        targets = find_targets(folder)
        for file in targets:
            target = file.name
            output = (pathlib.Path(args.output) / file.parent.name / target).with_suffix('.json')
            output.parent.mkdir(parents=True, exist_ok=True)
            if output.exists():
                continue
            tasks.append((args.tblgen, pathlib.Path(args.llvm), file, output))
    multiprocessing.Pool().starmap(tblgen, tasks)
            # tblgen(args.tblgen, pathlib.Path(args.llvm), file, output)
        
    
if __name__ == "__main__":
    main()