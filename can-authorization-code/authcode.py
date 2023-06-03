from bitstring import BitArray
import unittest
import csv, sys, os

def code(value:BitArray, init=0x00, config:BitArray=BitArray(hex='0000')):
    res = BitArray(hex='0000')

# BIT 0
    res[0] = value[0xb] != value[0xc]

    if(value[8]):
        res[0] = not res[0]

    if(value[4]):
        res[0] = not res[0]

    if(value[0]):
        res[0] = not res[0]

    if(not init and config[0]):
        res[0] = not res[0]
# BIT 0 END
# BIT 1
    res[1] = value[0xc] != value[0xd]

    if(value[9]):
        res[1] = not res[1]

    if(value[5]):
        res[1] = not res[1]

    if(value[1]):
        res[1] = not res[1]

    if(not init and config[1]):
        res[1] = not res[1]
# BIT 1 END
# BIT 2
    res[2] = value[0xd] != value[0xe]

    if(value[0xa]):
        res[2] = not res[2]

    if(value[6]):
        res[2] = not res[2]

    if(value[2]):
        res[2] = not res[2]

    if(not init and config[2]):
        res[2] = not res[2]
# BIT 2 END
# BIT 3
    res[3] = value[0xe] != value[0xf]

    if(value[0xb] or value.uint == 0x60f):
        res[3] = not res[3]

    if(value[7]):
        res[3] = not res[3]

    if(value[3]):
        res[3] = not res[3]

    if(not init and config[3]):
        res[3] = not res[3]
# BIT 3 END
# BIT 4
    res[4] = value[0xF]

    if(value[0xc] or value.uint == 0x60f):
        res[4] = not res[4]

    if(value[8]):
        res[4] = not res[4]

    if(value[4]):
        res[4] = not res[4]
    
    if(not init and config[4]):
        res[4] = not res[4]
# BIT 4 END
# BIT 5
    res[5] = value[0xc] != value[0xd]

    if(value[0xb] or value.uint == 0x60f):
        res[5] = not res[5]

    if(value[9]):
        res[5] = not res[5]

    if(value[8]):
        res[5] = not res[5]

    if(value[5]):
        res[5] = not res[5]

    if(value[4]):
        res[5] = not res[5]

    if(value[0]):
        res[5] = not res[5]

    if(not init and config[5]):
        res[5] = not res[5]
# BIT 5 END
# BIT 6
    res[6] = value[0xd] != value[0xe]

    if(value.uint == 0x60f):
        res[6] = not res[6]

    if(value[0xc]):
        res[6] = not res[6]
    
    if(value[0xa]):
        res[6] = not res[6]

    if(value[9]):
        res[6] = not res[6]

    if(value[6]):
        res[6] = not res[6]

    if(value[5]):
        res[6] = not res[6]

    if(value[1]):
        res[6] = not res[6]

    if(not init and config[6]):
        res[6] = not res[6]
# BIT 6 END
# BIT 7
    res[7] = value[0xe] != value[0xf]

    if(value[0xd]):
        res[7] = not res[7]

    if(value[0xb]):
        res[7] = not res[7]

    if(value[0xa]):
        res[7] = not res[7]

    if(value[7]):
        res[7] = not res[7]

    if(value[6]):
        res[7] = not res[7]

    if(value[2]):
        res[7] = not res[7]

    if(not init and config[7]):
        res[7] = not res[7]
# BIT 7 END
# BIT 8
    res[8] = value[0xe] != value[0xf]

    if(value[0xc]):
        res[8] = not res[8]

    if(value[0xb]):
        res[8] = not res[8]

    if(value[8]):
        res[8] = not res[8]

    if(value[7]):
        res[8] = not res[8]

    if(value[3]):
        res[8] = not res[8]

    if(not init and config[8]):
        res[8] = not res[8]
# BIT 8 END
# BIT 9
    res[9] = value[0xd] != value[0xf]

    if(value.uint == 0x60f):
        res[9] = not res[9]

    if(value[0xc]):
        res[9] = not res[9]

    if(value[9]):
        res[9] = not res[9]

    if(value[8]):
        res[9] = not res[9]

    if(value[4]):
        res[9] = not res[9]

    if(not init and config[9]):
        res[9] = not res[9]
# BIT 9 END
# BIT 10
    res[0xA] = value[0xd] != value[0xe]

    if(value[0xa]):
        res[0xA] = not res[0xA]

    if(value[9]):
        res[0xA] = not res[0xA]

    if(value[5]):
        res[0xA] = not res[0xA]

    if(not init and config[0xA]):
        res[0xA] = not res[0xA]
# BIT 10 END
# BIT 11
    res[0xB] = value[0xe] != value[0xf]

    if(value.uint == 0x60f):
        res[0xB] = not res[0xB]

    if(value[0xb]):
        res[0xB] = not res[0xB]

    if(value[0xa]):
        res[0xB] = not res[0xB]

    if(value[6]):
        res[0xB] = not res[0xB]

    if(not init and config[0xB]):
        res[0xB] = not res[0xB]
# BIT 11 END
# BIT 12
    res[0xC] = value[0xF]

    if(value[7] != value[8] or value.uint == 0x60F):
        res[0xC] = not res[0xC]

    if(value[4]):
        res[0xC] = not res[0xC]

    if(value[0]):
        res[0xC] = not res[0xC]

    if(not init and config[0xC]):
        res[0xC] = not res[0xC]
# BIT 12 END
# BIT 13
    res[0xD] = value[8] != value[9]
    
    if(value[5] or value.uint == 0x60f):
        res[0xD] = not res[0xD]

    if(value[1]):
        res[0xD] = not res[0xD]

    if(not init and config[0xD]):
        res[0xD] = not res[0xD]
# BIT 13 END
# BIT 14
    res[0xE] = value[9] != value[0xA]

    if(value[6] or value.uint == 0x60f):
        res[0xE] = not res[0xE]

    if(value[2]):
        res[0xE] = not res[0xE]

    if(not init and config[0xE]):
        res[0xE] = not res[0xE]
# BIT 14 END
# BIT 15
    res[0xF] = value[0xa] != value[0xb]
    
    if(value[7] or value.uint == 0x60f):
        res[0xF] = not res[0xF]

    if(value[3]):
        res[0xF] = not res[0xF]

    if(not init and config[0xF]):
        res[0xF] = not res[0xF]
# BIT 15 END

    return res

### TESTS
script_path = sys.argv[0] 
base_dir_pair = os.path.split(script_path)

init_test_file_name = os.path.join(base_dir_pair[0], '01_0000-FFFF_00_0000.csv')
calc_test_file_name = os.path.join(base_dir_pair[0], '01_C170_00_0000-FFFF.csv')

test_data_row_list = []
test_data_bitmask = BitArray(bin='1111 1111 1111 1111')

def load_test_data(file_name):
    global test_data_row_list
    test_data_row_list = []
    with open(file_name, 'r') as file:
        csv_reader = csv.reader(file, delimiter=',')
        for row in csv_reader:
            test_data_row_list.append(row)
        print('open and load data from "'+file_name+'" complete.')


class TestAuthorizationInitCode(unittest.TestCase):

    @classmethod
    def setUp(cls):
        load_test_data(init_test_file_name)
        print('TestAuthorizationInitCode setup complete')        

    def test_CodeInitCalculation(self):
        for row in test_data_row_list:
            input=BitArray(hex=row[0])
            output=BitArray(hex=row[1])
            config = code(input, 0x01)
            calculated = code(BitArray(hex='0000'), 0x00, config)
            result = calculated & test_data_bitmask
            expected = output & test_data_bitmask
            self.assertEqual(result, expected, ('error for input: ['+input.hex+']; was: '+result.bin+', but should be: '+expected.bin)) 

class TestAuthorizationCode(unittest.TestCase):

    @classmethod
    def setUp(cls):
        load_test_data(calc_test_file_name)
        print('TestAuthorizationCode setup complete')   

    def test_CodeCalculation(self):
        config = code(BitArray(hex='C170'), 0x01)
        for row in test_data_row_list:
            input=BitArray(hex=row[0])
            output=BitArray(hex=row[1])
            calculated = code(input, 0x00, config)
            result = calculated & test_data_bitmask
            expected = output & test_data_bitmask
            self.assertEqual(result, expected, ('error for input: ['+input.hex+']; was: '+result.bin+', but should be: '+expected.bin)) 

if __name__ == '__main__':
    unittest.main()
