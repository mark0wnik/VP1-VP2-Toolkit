import sys

def intTryParse(value):
    try:
        return int(value), True
    except ValueError:
        return value, False

def GetFourthByte(input):
    if (input > 10):
        return 0
    return {
        6: 3,
        7: 3,
        8: 0,
        9: 1
    }.get(input, input)

def GetThirdByte(input):
    if (input > 10):
        return 0
    return {
        6: 2,
        7: 2,
        8: 0,
        9: 1
    }.get(input, input)

def GetSecondByte(input):
    if (input > 10):
        return 0
    return {
        6: 1,
        7: 1,
        8: 0,
        9: 1
    }.get(input, input)

def GetFirstByte(input):
    if (input > 10):
        return 0
    return {
        6: 0,
        7: 0,
        8: 0,
        9: 1
    }.get(input, input)

def GetCode(sn):
    code = 1111
    sn_ = [((sn // 1000) & 0x0F),
           ((sn % 1000) // 100 & 0x0F),
           ((sn % 100) // 10 & 0x0F),
           ((sn % 10) & 0x0F)]
    code += (GetThirdByte(sn_[3]) * 10)
    code += (GetFirstByte(sn_[2]) * 1000)
    code += (GetFourthByte(sn_[1]))
    code += (GetSecondByte(sn_[0]) * 100)
    return code

for i in range(1, len(sys.argv)):
    arg = sys.argv[i]
    (sn, parsed) = intTryParse(arg)
    if not parsed:
        print("bad input (provide last 4 digits of serial number, or set of such numbers)")
        continue
    code = GetCode(sn)
    print(code)