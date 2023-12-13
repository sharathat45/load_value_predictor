from collections import OrderedDict
import sys

def process_trace_file(input_file, output_file):
    # Read and process the input file
    with open(input_file, 'r') as file:
        lines = file.readlines()

    pc_details = OrderedDict()
    for line in lines:
        if 'PC:' in line:
            pc_address = line.split('PC:')[1].split()[0]

            if pc_address not in pc_details:
                pc_details[pc_address] = []
            pc_details[pc_address].append(line.strip())  # Stripping leading/trailing whitespace

    # Write the grouped and formatted data to the output file
    with open(output_file, 'w') as file:
        file.write("Trace File Analysis\n")
        file.write("Generated on: [Insert Date Here]\n")
        file.write("-" * 40 + "\n")  # Separator line
        for pc, details in pc_details.items():
            file.write(f"PC : {pc}\n")
            for detail in details:
                file.write(f"    {detail}\n")  # Indented details
            file.write("\n" + "-" * 40 + "\n")  # Separator line

        file.write("End of File\n")

# Rest of the script remains the same


if __name__ == "__main__":
    input_file = "/home/ecegridfs/a/socet152/load_value_predictor/m5out/trace.out"
    output_file = "/home/ecegridfs/a/socet152/load_value_predictor/m5out/extract.out"
    process_trace_file(input_file, output_file)
