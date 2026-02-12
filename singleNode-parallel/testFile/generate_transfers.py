def generate_commands(input_filename, output_filename):
    commands = []
    with open(input_filename, 'r') as infile:
        for line in infile:
            parts = line.strip().split(',')
            if len(parts) >= 6:
                from_wallet = parts[2]
                to_wallet = parts[4]
                command = f"./walletClientMain transfer {from_wallet} pass {to_wallet} pass 0"
                commands.append(command)

    with open(output_filename, 'w') as outfile:
        outfile.write('\n'.join(commands))

if __name__ == "__main__":
    input_file = input("Enter the input file name: ")
    output_file = "G"+input_file
    generate_commands(input_file, output_file)
    print(f"Commands written to {output_file}")
