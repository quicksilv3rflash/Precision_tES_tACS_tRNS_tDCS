#this function converts mA values to appropriate DAC write values
def mA_2_DAC_write(value_in_mA):
    if(value_in_mA > 2.002): #check positive limit
        value_in_mA = 2.001
    if(value_in_mA < -2.002):#check negative limit
        value_in_mA = -2.001
    dacwrite = (int(round(((16383*1.0866)/5)*(2.5-value_in_mA)))).to_bytes(2,byteorder="big",signed=False)
    #That horrible line of code converts mA values into correct format
    #for the DAC. First it runs the values through the linear equation
    #that converts them to appropriate DAC values, then it ensures the
    #data are formatted in 16-bit unsigned integers.
    return dacwrite
