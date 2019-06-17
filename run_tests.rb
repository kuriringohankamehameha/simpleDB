describe 'database' do
	
    before do
	  #File.open "DATABASE.txt", "w"
    File.delete("DATABASE.txt")
    File.open "DATABASE.txt"
    end

    class Table
      @@size = 1300
      def table_size
        @@size
      end
    end

    table = Table.new

    def run_script(commands)
        raw_output = nil
    IO.popen("./test DATABASE.txt", "r") do |pipe|
      commands.each do |command|
        pipe.puts command
      end

      pipe.close_write

      # Read entire output
      raw_output = pipe.gets(nil)
    end
    raw_output.split("\n")
  end

  it 'keeps data after closing connection' do
    result1 = run_script([
      "insert 1 user1 person1@example.com",
      ".exit",
    ])
    expect(result1).to match_array([
      "DATABASE >Executed.",
      "DATABASE >",
    ])

   result2 = run_script([
     "select",
     ".exit",
   ])
    expect(result2).to match_array([
      "DATABASE >(1, user1, person1@example.com)",
      "Executed.",
     "DATABASE >",
    ])
  end



  it 'prints constants' do
    script = [
      ".constants",
      ".exit",
    ]
    result = run_script(script)

    expect(result).to match_array([
      "DATABASE >Constants:",
      "ROW_SIZE: 293",
      "COMMON_NODE_HEADER_SIZE: 6",
      "LEAF_NODE_HEADER_SIZE: 10",
      "LEAF_NODE_CELL_SIZE: 297",
      "LEAF_NODE_SPACE_FOR_CELLS: 4086",
      "LEAF_NODE_MAX_CELLS: 13",
      "DATABASE >",
    ])
  end
 end