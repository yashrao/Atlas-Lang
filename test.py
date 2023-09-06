
import os
import subprocess

COMPILE_CMD = ['./atlas', '--include', '.', '--output', 'test/out', '--run']
RUN_CMD = ['./test/out']

# NOTE: set to True to output all the tests into txt files for convenience
#       then you can set to false to actually run the tests
OUTPUT_EXPECTED_RESULTS = False

class Colors:
    GREEN = '\033[92m'
    RED = '\033[91m'
    RESET = '\033[0m'

def run_test(test_name):
    try:
        print(' '.join(COMPILE_CMD) + ' ' + test_name)
        res = subprocess.check_output(COMPILE_CMD + [test_name]).decode('utf-8')
        if OUTPUT_EXPECTED_RESULTS == True:
            output_file_name = test_name.split('.')[0] + '.txt'
            print('Writing file ' + output_file_name)
            with open(output_file_name, 'w') as f:
                f.write(res)
        else:
            expected_output = ''
            with open(test_name.split('.')[0] + '.txt') as f:
                expected_output = f.read()
            assert res == expected_output
            print('"' + test_name + '"' + Colors.GREEN + ' SUCCEEDED' + Colors.RESET + '\n')
    except subprocess.CalledProcessError as e:
        print('Error: Process exited with non-zero exit status for "' + test_name + '"')
        print(test_name)
    except FileNotFoundError as e:
        print('Missing "' + test_name.split('.')[0] + '.txt"')
        print('"' + test_name + '"' + ' has no expected output txt')
        print('Skipping...')
    except AssertionError as e:
        print('TEST FOR "' + test_name + '"' + Colors.RED + ' FAILED' + Colors.RESET)
        print('Expected Output:')
        print(expected_output, end='')
        print('-------------------')
        print('Actual Output:')
        print(res)
        print('')

def main():
    print('')
    directory = 'test'
    test_files = os.walk(directory)
    res = []
    for (dir_path, dir_names, file_names) in test_files:
        for file in file_names:
            if file.split('.')[-1] == 'atl':
                res.append(dir_path + '/' + file)

    for file in res:
        run_test(file)

if __name__ == '__main__':
    main()
