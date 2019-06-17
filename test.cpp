#include "test.h"
//Function Definitions

// A Hash function, taken from stackoverflow.com
#define A 54059 /* a prime */
#define B 76963 /* another prime */
#define C 86969 /* yet another prime */
#define FIRSTH 37 /* also prime */
uint32_t hash_str(const char* s)
{
   uint32_t h = FIRSTH;
   while (*s) {
     h = (h * A) ^ (s[0] * B);
     s++;
   }
   return h; // or return h % C;
}


uint32_t* leaf_node_num_cells(void* node)
{
	return (uint32_t*)(static_cast<char*>(node) + LEAF_NODE_NUM_CELLS_OFFSET);
}

void* leaf_node_cell(void* node, uint32_t cell_num)
{
	return (static_cast<char*>(node) + (LEAF_NODE_HEADER_SIZE + (cell_num)* (LEAF_NODE_CELL_SIZE)));
}

uint32_t* leaf_node_key(void* node, uint32_t cell_num)
{
	//return (uint32_t*)(static_cast<char*>(node + LEAF_NODE_HEADER_SIZE + (cell_num)*LEAF_NODE_CELL_SIZE - LEAF_NODE_KEY_SIZE);
	return (uint32_t*)((char*)leaf_node_cell(node, cell_num));
}

uint32_t* leaf_node_sibling(void* node)
{
	//Returns the address to the next leaf node of 'node'
	return (uint32_t*)(static_cast<char*>(node) + LEAF_NODE_SIBLING_OFFSET);
}

void* leaf_node_value(void* node, uint32_t cell_num)
{
	return (void*)(static_cast<char*>(leaf_node_cell(node, cell_num)) + LEAF_NODE_KEY_SIZE);
}

void initialize_leaf_node(void* node)
{
	//Set the node to Zero
	set_node_type(node, NODE_LEAF);
	//Set is_root to false
	set_node_root(node, false);
	*(leaf_node_num_cells(node)) = 0;
	*(leaf_node_sibling(node)) = 0;
}

void initialize_internal_node(void* node)
{
	set_node_type(node, NODE_INTERNAL);
	set_node_root(node, false);
	*(internal_node_num_keys(node)) = 0;
}


class Cursor* table_start(Table* table)
{
	//class Cursor* cursor = (class Cursor*) malloc(sizeof(class Cursor));
	class Cursor* cursor = new Cursor(table);

	//Addition : The Cursor must point to the key of the lowest indexed leaf node available
	cursor = table_find(table, 0); //This gives the next lowest key, even if the key 0 didn't exist

	void* root_node = get_page(table->pager, table->root_page_num);
	uint32_t num_cells = *leaf_node_num_cells(root_node);
	cursor->setEOT(num_cells == 0);

	return cursor;
}

class Cursor* table_end(Table* table)
{
	class Cursor* cursor = (class Cursor*) malloc(sizeof(class Cursor));
	cursor = new Cursor(table);
	cursor->setPageNum(table->root_page_num);
	void* root_node = get_page(table->pager, table->root_page_num);
	uint32_t num_cells = *leaf_node_num_cells(root_node);
	cursor->setCellNum(num_cells);
	cursor->setEOT(true);
	return cursor;
}

//Allocating Rows now happens through declaration of cursors
void* cursor_value(class Cursor* cursor)
{
	//uint32_t row_num = cursor->getRow();
	//uint32_t page_num = row_num / ROWS_PER_PAGE;
	uint32_t page_num = cursor->getPage();
	void* page = get_page(cursor->getTable()->pager, page_num);
	//uint32_t row_offset = row_num % ROWS_PER_PAGE;
	//uint32_t byte_offset = row_offset * ROW_SIZE;

	//return (void*)(static_cast<char*>(page + byte_offset);
	return leaf_node_value(page, cursor->getCellNum());
}

void cursor_advance(Cursor* cursor)
{
	//Change : Add this before advancing
	//Basically, get the current page number first
	uint32_t page_num = cursor->getPage();
	void* node = get_page(cursor->getTable()->pager, page_num);

	//Advances the cursor by a row and sets EOT if needed
	cursor->increment();

	if (cursor->getCellNum() >= (*leaf_node_num_cells(node)))
	{
		//The cursor now jumps to the sibling, until it has reached the end of the leaf nodes, i.e, EOT, when that page_num = 0 (initial conditions)
		uint32_t next_page = *(leaf_node_sibling(node));

		if (next_page == 0)
		{
			//End Of Table
			cursor->setEOT(true);
		}

		else
		{
			//Jump to next leaf node (right sibling)
			cursor->setPageNum(next_page);
			cursor->setCellNum(0);
		}
	}

	/*
	if (cursor->getRow() >= cursor->getTable()->num_rows)
	{
		cursor->setEOT();
	}
	*/

	return;
}

InputBuffer* new_input_buffer()
{
	//Creates an InputBuffer stream
	//InputBuffer* input_buffer = (InputBuffer*)malloc(sizeof(InputBuffer));
	InputBuffer* input_buffer = new InputBuffer();

	input_buffer->buffer = "";
	input_buffer->buffer_length = 0;
	input_buffer->input_length = 0;

	return input_buffer;
}

void print_prompt()
{
	//Generic Prompt
	cout << "IRCS-TN >";
	return;
}

istream& my_getline(istream& is, std::string& s, char delim = '\n')
{
	//getline returns an istream type which can be accessed by the object cin
	s.clear();

	int ch;

	while ((ch = is.get()) != EOF && ch != delim)
	{
		s.push_back(ch);
	}

	return is;
}

uint32_t my_ascii_to_integer(string buffer)
{
	//Returns an integer and makes sure only 'integer' strings can be converted
	const char* buf = (char*)(malloc(sizeof(char)));
	buf = buffer.c_str();

	int flag = 1;

	for (int i = 0; buf[i] != '\0'; i++)
	{
		int ascii_value = buf[i] - '0';
		//cout << "ascii value for " << buf[i] <<" is " << ascii_value << endl;
		if (!(ascii_value >= 0 && ascii_value <= 9))
		{
			//Not OK
			flag = 0;
			break;
		}
	}
	if (flag == 0)
		return -1;

	return (uint32_t)atoi(buf);

}

void read_input(InputBuffer* input_buffer)
{
	//Parses the input from the buffer

	//We use getline() here, to get the size of the line from buffer
	//int bytes_read = my_getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin);
	istream& is = my_getline(std::cin, input_buffer->buffer);

	size_t bytes_read = input_buffer->buffer.size();

	//cout << "Bytes read = " << bytes_read << endl;

   //Handle cases
	if (bytes_read <= 0)
	{
		//cout << endl;
		//printf("Error reading Input\n");
		//exit(-1);
	}

	return;
}

void close_input_buffer(InputBuffer* input_buffer)
{
	//Closes the InputBuffer
	input_buffer->buffer.clear();
	free(input_buffer);

	return;
}

uint32_t get_password(const char* filename = "password.txt")
{
	ifstream file;
	file.open(filename);

	char buffer[255];

	file >> buffer;

	file.close();

	return my_ascii_to_integer(buffer);
}

void write_password(uint32_t password, const char* filename = "password.txt")
{
	 if (remove(filename) == -1) {  // remove() returns -1 on error
      cerr << "Error: " << strerror(errno) << endl;
      exit(EXIT_FAILURE);
   }
	//Create a new file with the same name
	string string_password;
	string_password = to_string(password);

	ofstream myfile;
	myfile.open(filename);
	myfile << string_password;
	myfile.close();

}

PrepareResult password_prompt()
{    
    char* input = new char();

	if(potential_attacker == true)
	{
		input = getpass("Enter Password:");

		//Possibly generate logs to a file ????

		if(hash_str(input) == PASS)
        {
            return PREPARE_PASSWORD_SUCCESS;
        }

		if(waiting_time <= UINT32_MAX/2)
		waiting_time *= 2;

		return PREPARE_PASSWORD_FAILURE;

	}

    uint8_t number_of_attempts = 4;

    while(number_of_attempts > 0)
    {
		if(SYSTEM == 1)
        input = getpass("Enter Password:");

		else
		{
			/* TODO: Windows input	*/
		
		}
        
        if(hash_str(input) == PASS)
        {
            //delete input;
            return PREPARE_PASSWORD_SUCCESS;
        }
        
        number_of_attempts--;
        
        if(number_of_attempts > 0)
            cout << "Incorrect Password. Please try again.\n";
    }

    //delete input;

    return PREPARE_PASSWORD_FAILURE;
}

bool waiting_period(time_t time)
{
	//Can type password again only if time interval > 60 seconds (1 min)
	if(time - current_time > waiting_time)
	{
		return true;
	}

	return false;
}

uint32_t get_num_records(Table* table, char* filename)
{
	ifstream file;
	file.open(filename);

	char buffer[255];

	file >> buffer;

	file.close();

	return my_ascii_to_integer(buffer);
}

PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement)
{
	//Reads the statement from buffer

	//if (strncmp(input_buffer->buffer, "insert", 6) == 0)
	if (input_buffer->buffer.compare(0, 6, "insert") == 0)
	{
        //Prompt for Password if normal user
		if(i_am_root == false)
		{
			if(password_prompt() == PREPARE_PASSWORD_FAILURE)
			{
				return PREPARE_PASSWORD_FAILURE;
			}
		}

		statement->type = STATEMENT_INSERT;

		vector<string> tokens;

		input_buffer->buffer.push_back(' ');

		stringstream check(input_buffer->buffer);

		string intermediate;

		int args = 0;

		while (my_getline(check, intermediate, ' '))
		{
			if (args > 0)
				tokens.push_back(intermediate);
			args++;
		}

		if (args != 4)
		{
			cout << "You have supplied " << args << " arguments. Please supply 4 arguments." << endl;

			return PREPARE_SYNTAX_ERROR;
		}

		statement->row_to_insert.id = my_ascii_to_integer(tokens[0]);

		//cout << "Statement->row_to_insert.id = " << statement->row_to_insert.id << endl;

		if (statement->row_to_insert.id == UINT_MAX)
		{
			return PREPARE_NEGAGIVE_ID;
		}

		strcpy(statement->row_to_insert.username, tokens[1].c_str());

		if (strlen(statement->row_to_insert.username) > COLUMN_USERNAME_SIZE)
		{
			return PREPARE_USERNAME_OVERFLOW;
		}

		strcpy(statement->row_to_insert.email, tokens[2].c_str());

		if (strlen(statement->row_to_insert.email) > COLUMN_EMAIL_SIZE)
		{
			return PREPARE_EMAIL_OVERFLOW;
		}

		return PREPARE_SUCCESS;
	}

	else if (input_buffer->buffer.compare(0, 6, "select") == 0)
	{
		statement->type = STATEMENT_SELECT;

		return PREPARE_SUCCESS;
	}

	else if (input_buffer->buffer.compare(0, 6, "delete") == 0)
	{
		statement->type = STATEMENT_DELETE;

		vector<string> tokens;

		input_buffer->buffer.push_back(' ');

		stringstream check(input_buffer->buffer);

		string intermediate;

		int args = 0;

		while (my_getline(check, intermediate, ' '))
		{
			if (args > 0)
				tokens.push_back(intermediate);
			args++;
		}

		if (args != 2)
		{
			cout << "You have supplied " << args - 1 << " arguments. Please supply 1 argument." << endl;

			return PREPARE_SYNTAX_ERROR;
		}

		statement->row_to_insert.id = my_ascii_to_integer(tokens[0]);

		//cout << "Statement->row_to_insert.id = " << statement->row_to_insert.id << endl;

		if (statement->row_to_insert.id == UINT_MAX)
		{
			return PREPARE_NEGAGIVE_ID;
		}

		return PREPARE_SUCCESS;

	}

	else if (input_buffer->buffer.compare(0,1, "") == 0)
	{
		return PREPARE_EMPTY;
	}

	return PREPARE_UNRECOGNIZED_STATEMENT;
}

//Storage Plan:

//Group block of memory into pages
//Set Page size limits 
//Pages are allocated as per requirement
//Keep references to pages, and store them as an array of pointers

//There must be a way to serialize objects 

void serialize_row(Row* source, void* destination)
{
	//Reference:
	//memcpy(destination, source, number_of_bytes)

	//Copy Row ID
	memcpy((static_cast<char*>(destination) + ID_OFFSET), &(source->id), ID_SIZE);

	//Copy Row Username
	memcpy(static_cast<char*>(destination) + USERNAME_OFFSET, &(source->username), USERNAME_SIZE);

	//Copy Row Email
	memcpy(static_cast<char*>(destination) + EMAIL_OFFSET, &(source->email), EMAIL_SIZE);

	//NOTE: Since void ptr arithmetic isn't allowed, convert it to char* before jumping by OFFSET amount of locations

	return;

}

void deserialize_row(void* source, Row* destination)
{
	//Deserializes the row from memory
	memcpy(&(destination->id), (static_cast<char*>(source) + ID_OFFSET), ID_SIZE);
	memcpy(&(destination->username), (static_cast<char*>(source) + USERNAME_OFFSET), USERNAME_SIZE);
	memcpy(&(destination->email), (static_cast<char*>(source) + EMAIL_OFFSET), EMAIL_SIZE);

	return;
}

void* get_page(Pager* pager, uint32_t page_num)
{
	if (page_num > TABLE_MAX_PAGES)
	{
		cout << "Tried to fetch page number out of bounds. " << page_num << " > " << TABLE_MAX_PAGES << endl;
		exit(EXIT_FAILURE);
	}

	if (pager->pages[page_num] == NULL)
	{
		//Allocate memory and load from the file
		void* page = malloc(PAGE_SIZE);
		uint32_t num_pages = pager->file_length / PAGE_SIZE;

		//We may store a parital page at EOF
		if (pager->file_length % PAGE_SIZE)
		{ 
			//Allocate an extra partial page for the remaining space unused (Lol)
			num_pages++;
		}

		if (page_num <= num_pages)
		{
			lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
			ssize_t bytes_read = read(pager->file_descriptor, page, PAGE_SIZE);

			if (bytes_read == -1)
			{
				cout << "Error reading file: " << errno << endl;
				exit(EXIT_FAILURE);
			}
		}

		pager->pages[page_num] = page;

		if (page_num >= pager->num_pages)
		{
			pager->num_pages = page_num + 1;
		}
	}

	return pager->pages[page_num];
}


void print_row(Row* row)
{
	//Prints the row
	cout << "(" << row->id << ", " << row->username << ", " << row->email << ")" << endl;

	return;
}

void add_padding(Pager* pager, uint32_t page_num, const char* filename, uint32_t file_length)
{
	if (file_length % PAGE_SIZE != 0)
	{
		uint32_t diff = file_length % PAGE_SIZE;
		ssize_t bytes_written = write(pager->file_descriptor, pager->pages[page_num], diff);
	}
}

Pager* pager_open(const char* filename)
{
	int fd = open(filename, O_RDWR | O_CREAT, 0444);

	//fstream fs;

	//fs.open(filename, fstream::in | fstream::out | fstream::app);

	if (fd == -1)
	{
		cout << "Unable to open file\n";
		exit(EXIT_FAILURE);
	}

	//Find the initial file size when first opened
	uint32_t file_length = lseek(fd, 0, SEEK_END);

	Pager* pager = new Pager();

	pager->file_descriptor = fd;
	pager->file_length = file_length;
	pager->num_pages = file_length / PAGE_SIZE;

	if (file_length % PAGE_SIZE != 0)
	{
		//write(pager->file_descriptor, filename, ' ', file_length % PAGE_SIZE);
		cout << "CorruptFile Error: Database File Length is not a multiple of the Page Size\n";
		exit(EXIT_FAILURE);

	}

	//cout << "File Size is " << file_length << " bytes." << endl;

	for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++)
	{
		pager->pages[i] = NULL;
	}

	return pager;

}

void pager_flush(Pager* pager, uint32_t page_num)
{
	if (pager->pages[page_num] == NULL)
	{
		cout << "Tried to flush a NULL page.\n";
		exit(EXIT_FAILURE);
	}

	//Move to offset of the page number
	int offset = lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);

	if (offset == -1)
	{
		cout << "Error seeking: " << errno << endl;
		exit(EXIT_FAILURE);
	}

	//Write the whole page containing SIZE bytes 
	ssize_t bytes_written = write(pager->file_descriptor, pager->pages[page_num], PAGE_SIZE);

	if (bytes_written == -1)
	{
		cout << "Error writing: " << errno << endl;
		exit(EXIT_FAILURE);
	}
}

Table* db_open(const char* filename)
{
	//Opens the db file and opens a new connection
	Pager* pager = pager_open(filename);

	//uint32_t num_rows = pager->file_length / ROW_SIZE;

	Table* table = new Table();
	//table->num_rows = num_rows;
	table->pager = pager;

	table->root_page_num = 0;

	if (pager->num_pages == 0)
	{
		//Initialize page 0 as leaf node
		void* root_node = get_page(pager, 0);
		initialize_leaf_node(root_node);

		//Set root node
		set_node_root(root_node, true);
	}

	//Retrieve number of records from the 'num_rows.txt'
	table->num_rows = get_num_records(table, "num_rows.txt");

	return table;
}

void db_close(Table* table)
{
	Pager* pager = table->pager;

	//uint32_t num_full_pages = table->num_rows / ROWS_PER_PAGE;
	uint32_t num_full_pages = pager->num_pages;

	for (uint32_t i = 0; i < num_full_pages; i++)
	{
		if (pager->pages[i] == NULL)
		{
			continue;
		}

		//Now flush the cache of existing pages into disk
		pager_flush(pager, i);
		free(pager->pages[i]);
		pager->pages[i] = NULL;
	}

	if (CLOSE(pager->file_descriptor) == -1)
	{
		cout << "Error closing DATABASE file. \n";
		exit(EXIT_FAILURE);
	}

	for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++)
	{
		void* page = pager->pages[i];

		//Free all pages
		if (page != NULL)
		{
			free(page);
			pager->pages[i] = NULL;
		}
	}

	free(pager);

	ofstream file;
	file.open("num_rows.txt");
	file << table->num_rows;
	file.close();

	free(table);

}

void mymemcpy(void *dest, void *src, size_t n) 
{ 
   // Typecast src and dest addresses to (char *) 
   char *csrc = (char *)src; 
   char *cdest = (char *)dest; 
  
   // Copy contents of src[] to dest[] 
   for (int i=0; i<n; i++) 
       cdest[i] = csrc[i]; 
}

void leaf_node_insert(class Cursor* cursor, uint32_t key, Row* value)
{
	//cout << "Inserting " << key << " in leaf node...\n";
	void* node = get_page(cursor->getTable()->pager, cursor->getPageNum());
	uint32_t num_cells = *leaf_node_num_cells(node);

	if (num_cells >= LEAF_NODE_MAX_CELLS)
	{
		//cout << "Need to Implement Splitting of Leaf Node\n";
		//exit(EXIT_FAILURE);
		split_leaf_and_insert(cursor, key, value);
		return;

	}

	if (cursor->getCellNum() < num_cells)
	{
		//create a new cell
		//serialize_row(value, leaf_node_cell(node, num_cells));
		//*(leaf_node_key(node, num_cells))
		//Make room for the new cell

		uint32_t i;

		uint32_t* temp = leaf_node_key(node, cursor->getCellNum());

		//Declare a new cell
		void* new_cell = malloc(sizeof(void*));
		new_cell = leaf_node_cell(node, num_cells);

		for (i = num_cells; i > cursor->getCellNum(); i--)
		{
			mymemcpy(leaf_node_cell(node, i), leaf_node_cell(node, i - 1), LEAF_NODE_CELL_SIZE);
		}
	}

	*(leaf_node_num_cells(node)) += 1;
	*(leaf_node_key(node, cursor->getCellNum())) = key;
	serialize_row(value, leaf_node_value(node, cursor->getCellNum()));
	cursor->getTable()->num_rows += 1;
}

uint32_t get_unused_page_num(Pager* pager)
{
	return pager->num_pages;
}

bool is_root(void* node)
{
	//Checks if the given node is a root node or not
	uint8_t value = *((uint8_t*)(static_cast<char*>(node) + IS_ROOT_OFFSET));

	return (bool)value;
}

void set_node_root(void* node, bool is_root)
{
  uint8_t value = is_root;
  *((uint8_t*)((char*)node + IS_ROOT_OFFSET)) = value;
}

uint32_t* internal_node_num_keys(void* node)
{
	return (uint32_t*)(static_cast<char*>(node) + INTERNAL_NODE_NUM_KEYS_OFFSET);
}

uint32_t get_node_max_key(void* node)
{
	//For an internal node, the maximum key is it's right key
	if (get_node_type(node) == NODE_INTERNAL)
	{
		uint32_t last_key = *(internal_node_num_keys(node)) - 1;

		uint32_t rightmost_node_key = *internal_node_key(node, last_key);

		return rightmost_node_key;
	}

	//For a leaf node, the maximum key is at the maximum (right-most) index
	else if (get_node_type(node) == NODE_LEAF)
	{
		uint32_t num_cells = *(leaf_node_num_cells(node));

		uint32_t last_cell_num = num_cells - 1;

		uint32_t rightmost_node_key = *leaf_node_key(node, last_cell_num);

		return rightmost_node_key;
	}

	return 0;

}

uint32_t* internal_node_cell(void* node, uint32_t cell_num)
{
	return (uint32_t*)(static_cast<char*>(node) + INTERNAL_NODE_HEADER_SIZE + cell_num * INTERNAL_NODE_CELL_SIZE);
}

uint32_t* internal_node_key(void* node, uint32_t key_num)
{
	return (uint32_t*)(static_cast<char*>((void*)internal_node_cell(node, key_num)) + INTERNAL_NODE_CHILD_SIZE);
}

uint32_t* internal_node_right_child(void* node)
{
	return (uint32_t*)(static_cast<char*>(node) + INTERNAL_NODE_RIGHT_CHILD_OFFSET);
}

uint32_t* internal_node_child(void* node, uint32_t child_num)
{
	//Searches for the child with child_num = child_num
	uint32_t num_keys = *internal_node_num_keys(node);
	
	if (child_num > num_keys)
	{
		cout << "Tried to access child_num: " << child_num << ", but num_keys = " << num_keys << endl;
		exit(EXIT_FAILURE);
	}

	else if (child_num == num_keys)
	{
		//Return rightmost child(last entry) of the node 
		return internal_node_right_child(node);
	}

	else
	{
		//return (uint32_t*)(static_cast<char*>(node + INTERNAL_NODE_HEADER_SIZE + child_num*(INTERNAL_NODE_CHILD_SIZE));
		return internal_node_cell(node, child_num);
	}


}

void update_internal_node_key(void* node, uint32_t old_key, uint32_t new_key)
{
	uint32_t old_child_index = find_internal_node_child(node, old_key);
	*internal_node_key(node, old_child_index) = new_key;
}

uint32_t find_median_key(void* node, uint32_t node_page_num)
{
	uint32_t num_keys = *internal_node_num_keys(node);

	return *(internal_node_key(node, (num_keys + 1) / 2));
}

void* get_internal_node_from_key(void* node, uint32_t key, uint32_t max_keys, int32_t page_num)
{
	//Returns a node, given it's key and page number
	for (uint32_t i = 0; i < max_keys; i++)
	{
		void* temp = ((void*)(static_cast<char*>(node) + INTERNAL_NODE_CHILD_OFFSET + i*INTERNAL_NODE_CHILD_SIZE));
		if (*internal_node_key(temp, i) == key)
		{
			return temp;
		}
	}

}

void internal_node_insert(Table* table, uint32_t parent_page_num, uint32_t child_page_num)
{
	cout << "Inserting in internal node...\n";
	void* parent = get_page(table->pager, parent_page_num);
	void* child = get_page(table->pager, child_page_num);
	uint32_t child_max_key = get_node_max_key(child);
	uint32_t index = find_internal_node_child(parent, child_max_key);
    
    uint32_t index_key = *internal_node_key(child, index);

	uint32_t original_num_keys = *internal_node_num_keys(parent);
	*internal_node_num_keys(parent) = original_num_keys + 1;

	cout << "Parent has " << original_num_keys << "number of keys , and max_cells_internal_node = " << INTERNAL_NODE_MAX_CELLS << endl;
	
	if (original_num_keys >= INTERNAL_NODE_MAX_CELLS)
	{
		 if(is_root(parent))
          printf("Parent is a root\n");
		split_internal_node(table, parent, parent_page_num);
	}

	uint32_t right_child_page_num = *internal_node_right_child(parent);
	void* right_child = get_page(table->pager, right_child_page_num);
	
	if (child_max_key > get_node_max_key(right_child))
	{
		/* Replace right child */
		*internal_node_child(parent, original_num_keys) = right_child_page_num;
		*internal_node_key(parent, original_num_keys) = get_node_max_key(right_child);
		*internal_node_right_child(parent) = child_page_num;
		
	}
	
	else 
	{
		/* Make room for the new cell */
		for (uint32_t i = original_num_keys; i > index; i--)
		{
			void* destination = internal_node_cell(parent, i);
			void* source = internal_node_cell(parent, i - 1);
			memcpy(destination, source, INTERNAL_NODE_CELL_SIZE);	
		}

		*internal_node_child(parent, index) = child_page_num;
		*internal_node_key(parent, index) = child_max_key;
	}

}

uint32_t* node_parent(void* node)
{
	return (uint32_t*)(static_cast<char*>(node) + PARENT_POINTER_OFFSET);
}

void create_new_root(Table* table, uint32_t right_child_page_num)
{
	//Creates a new root node with it's page num set to page_num
	//The Old Root is copied to new page, and becomes left child
	cout << "Creating new root...\n";
	void* root = get_page(table->pager, table->root_page_num);
	void* right_child = get_page(table->pager, right_child_page_num);

	uint32_t left_child_page_num = get_unused_page_num(table->pager);
	void* left_child = get_page(table->pager, left_child_page_num);

	//Copy the old root to the left child
	memcpy(left_child, root, PAGE_SIZE);
	
	//The old root is no longer the root, as it is the left child
	set_node_root(left_child, false);

	//Now, the root page must be set to have one key and two children
	initialize_internal_node(root);
	set_node_root(root, true);
	*internal_node_num_keys(root) = 1;
	*internal_node_child(root, 0) = left_child_page_num;
	//*internal_node_child(root, 1) = right_child_page_num;

	uint32_t left_child_max_key = get_node_max_key(left_child);
	*internal_node_key(root, 0) = left_child_max_key;
	*internal_node_right_child(root) = right_child_page_num;
	*node_parent(left_child) = table->root_page_num;
	*node_parent(right_child) = table->root_page_num;
}


void split_leaf_and_insert(class Cursor* cursor, uint32_t key, Row* value)
{
	//This splits the leaf, based on the keys, if it is full.
	/*
		(ROOT){1,a}, {2,b}, {3,c} -> (ROOT){2,b} -> RCHILD1{1,a},{2,b}
												  -> RCHILD2{3,c}
	*/

	cout << "SPLITTING LEAF NODE...\n";

	Table* table = cursor->getTable();

	void* old_node = get_page(cursor->getTable()->pager, cursor->getPageNum());

	uint32_t old_max = get_node_max_key(old_node);

	uint32_t new_page_num = get_unused_page_num(cursor->getTable()->pager);

	void* new_node = get_page(cursor->getTable()->pager, new_page_num);

	initialize_leaf_node(new_node);

	*node_parent(new_node) = *node_parent(old_node);

	//Update siblings whenever a split occurs

	//First set new->next as old->next
	*(leaf_node_sibling(new_node)) = *(leaf_node_sibling(old_node));
	//Then set old->next as new (new_page_num)
	*(leaf_node_sibling(old_node)) = new_page_num;

	//Copy every cell into it's new location
	//Notice the existence of an extra cell for the node to be inserted
	uint8_t flag = 1;

	for (int32_t i = LEAF_NODE_MAX_CELLS; i >= 0; i--)
	{
		void* destination_node;
		if (i >= LEAF_NODE_LEFT_SPLIT_COUNT)
		{
			destination_node = new_node;
		}

		else
		{
			destination_node = old_node;
		}

		uint32_t index_within_node = i % LEAF_NODE_LEFT_SPLIT_COUNT;

		void* destination = leaf_node_cell(destination_node, index_within_node);

		//Now serialize the row to point to the new location based on Cursor position

		if (i == cursor->getCellNum())
		{
			//Point of insertion
			//serialize_row(value, destination);

			//First, copy the key, and then the value
			serialize_row(value, leaf_node_value(destination_node, index_within_node));
			*(leaf_node_key(destination_node, index_within_node)) = key;

		}

		else if (i > cursor->getCellNum())
		{
			//The cells after the point of insertion must be copied to their new position, i.e ,
			// Old Position: K , New Position: K+1, so the i-1th cell of the old page must be in the i th position in the updated page
			//The for loop is taken in the reverse order for this reason
			memcpy(destination, leaf_node_cell(old_node, i-1), LEAF_NODE_CELL_SIZE);
		}

		else
		{
			//The previous cells do not need to be shifted, but rather simply copied
			memcpy(destination, leaf_node_cell(old_node, i), LEAF_NODE_CELL_SIZE);
		}

	}


	//Now update the cell counts in each header
	*(leaf_node_num_cells(old_node)) = LEAF_NODE_LEFT_SPLIT_COUNT;
	*(leaf_node_num_cells(new_node)) = LEAF_NODE_RIGHT_SPLIT_COUNT;

	//Finally, update the node's parents
	if (is_root(old_node))
	{
		//Create a new root, as the old one is now a leaf node
		return create_new_root(cursor->getTable(), new_page_num);
	
	}

	else
	{
		//cout << "Need to implement updating node's parents" << endl;
		//exit(EXIT_FAILURE);

		uint32_t parent_page_num = *node_parent(old_node);
		uint32_t new_max = get_node_max_key(old_node);
		void* parent = get_page(cursor->getTable()->pager, parent_page_num);

		update_internal_node_key(parent, old_max, new_max);
		internal_node_insert(cursor->getTable(), parent_page_num, new_page_num);
		return;
	}

}

MetaCommandResult do_meta_command(InputBuffer* input_buffer, Table* table)
{
	//if (strcmp(input_buffer->buffer, ".exit") == 0)
	if (input_buffer->buffer.compare(".exit") == 0)
	{
		i_am_root = false;
		db_close(table);
		exit(EXIT_SUCCESS);
	}

	else if (input_buffer->buffer.compare(".constants") == 0)
	{
		cout << "Constants:\n";
		print_constants();
		return META_COMMAND_SUCCESS;
	}

	else if (input_buffer->buffer.compare(".count") == 0)
	{
		cout << "Number of records in the table : " << table->pager->file_length / ROW_SIZE << endl;
		return META_COMMAND_SUCCESS;
	}

	else if (input_buffer->buffer.compare(".btree") == 0)
	{
		cout << "Tree:\n";
		//print_leaf_node(get_page(table->pager, 0));
		print_tree(table->pager, 0, 0);
		return META_COMMAND_SUCCESS;
	}

	else if (input_buffer->buffer.compare(".root") == 0)
	{
		if(potential_attacker && (time(0) - current_time <= waiting_time))
			return META_COMMAND_PASSWORD_FAILURE;

		//Logging in as root
		cout << "Logging in as root....\n";
		if(password_prompt() == PREPARE_PASSWORD_FAILURE)
		{
			return META_COMMAND_PASSWORD_FAILURE;
		}

		i_am_root = true;

		return META_COMMAND_EMPTY;
	}

	else if (input_buffer->buffer.compare(".password") == 0)
	{
		if(potential_attacker && (time(0) - current_time <= waiting_time))
			return META_COMMAND_PASSWORD_FAILURE;

		//Change the password
		cout << "Do you want to change your password?(y/n)\n";
		char ch;
		cin >> ch;
		if(ch == 'y')
		{
			if(password_prompt() == PREPARE_PASSWORD_SUCCESS)
			{
				//Change password
				char* new_pass = getpass("Enter new password:");
				uint32_t pass = hash_str(new_pass);
				char* new_pass_2 = getpass("Enter new password again:");
				if(hash_str(new_pass_2) == pass)
				{
					//Change password
					PASS = pass;
					write_password(PASS);
				}
				
				//delete new_pass;
				//free(new_pass);
			}
		}

	}

	else
	{
		return META_COMMAND_UNRECOGNIZED;
	}
}

class Cursor* find_leaf_node(Table* table, uint32_t page_num, uint32_t key)
{
	void* node = get_page(table->pager, page_num);
	uint32_t num_cells = *(leaf_node_num_cells(node));

	class Cursor* cursor = new Cursor(table);
	cursor->setPageNum(page_num);
	cursor->setEOT(false);

	//We utilize Binary Search to search for key in O(lg(n)) time
	uint32_t min = 0;
	uint32_t one_past_max = num_cells;

	while (min != one_past_max)
	{
		//cursor->setPageNum((min + one_past_max)/2);
		
		uint32_t mid = (min + one_past_max) / 2;

		if (key == *leaf_node_key(node, mid))
		{
			cursor->setCellNum(mid);
			return cursor;
		}

		else if (key < *leaf_node_key(node, mid))
		{
			//It is in the first half
			one_past_max = mid;
		}

		else
		{
			min = mid + 1;
		}
	}
	 
	cursor->setCellNum(min);
	return cursor;
}

NodeType get_node_type(void* node)
{
	uint8_t value = *(uint8_t*)(static_cast<char*>(node) + NODE_TYPE_OFFSET);
	return (NodeType)value;
}

void set_node_type(void* node, NodeType type)
{
	//Set Node Size
	uint8_t value = type;
	*(uint8_t*)(static_cast<char*>(node) + NODE_TYPE_OFFSET) = value;
}

uint32_t find_internal_node_child(void* node, uint32_t key)
{
	//Recursively searches for an internal node starting from the root for key, across the tree
	//If key does not exist, it returns a cursor to that location

	//uint32_t page_num = root_page_num;

	//void* node = get_page(table->pager, page_num);
	uint32_t num_keys = *(internal_node_num_keys(node));

	//Again, use Binary Search to find the key
	//These are offsets to add to the key_ptr variable to get the key number 
	uint32_t min = 0;
	uint32_t max = num_keys; // As num_keys = num_children + 1

	while (min != max)
	{
		//Indices 0 to num_keys/2 -1 is in the left child. The rest are in the right child

		uint32_t index = (min + max) / 2;

		//Smallest key in the right child
		uint32_t key_to_right = *(internal_node_key(node, index));

		if (key_to_right >= key)
		{
			//Node is in the left child
			max = index;
		}

		else
		{
			//It is in the right child
			min = index + 1;
		}

	}

	return min;
}


class Cursor* find_internal_node(Table* table, uint32_t page_num, uint32_t key)
{
	void* node = get_page(table->pager, page_num);	
	uint32_t child_index = find_internal_node_child(node, key);
	uint32_t child_num = *internal_node_child(node, child_index);

	void* child_node = get_page(table->pager, child_num);

	if (get_node_type(child_node) == NODE_LEAF)
	{
		//It is a leaf node. We have arrived at our destination!
		return find_leaf_node(table, child_num, key);
	}
	
	else
	{
		//It is an internal node. Recurse down the tree until you encounter a leaf node
		return find_internal_node(table, child_num, key);
		
	}
}

class Cursor* table_find(Table* table, uint32_t key)
{
	//Returns a cursor pointing to the location where the key needs to be inserted, in sorted order.

	uint32_t root_page_num = table->root_page_num;
	void* root_node = get_page(table->pager, root_page_num);

	if (get_node_type(root_node) == NODE_LEAF)
	{
		return find_leaf_node(table, root_page_num, key);
	}

	else
	{
		return find_internal_node(table, root_page_num, key);
	}
}

void split_internal_node(Table* table, void* node, uint32_t node_page_num)
{
	//Splits the internal node when it is full, into two nodes
	//The middle key is inserted into it's parent

	cout << "Splitting Internal Node..\n";
    
    if(is_root(node))
    {
        //Insert to root and possibly split
        uint32_t num_keys = *(internal_node_num_keys(node));
        cout << "Node has " << num_keys << " num of keys\n";
        
        if(num_keys > INTERNAL_NODE_MAX_CELLS)
        {
            cout << "Splitting Root...\n";
            //create_new_root(table, node_page_num);
        }
        
        else
        {
            cout << "No split? This shouldn't happen.\n";
        }
        
        return;
        
    }

	uint32_t node_max_keys = get_node_max_key(node);

	//Get the parent node
	uint32_t parent = *node_parent(node);

	//Get the index of the node from it's maximum key + parent's reference
	//uint32_t index = find_internal_node_child(node, internal_node_key(node, ));

	//Get the median key
	uint32_t median_key = find_median_key(node, node_max_keys);

	//void* median_node = get_internal_node_from_key(node, median_key, node_max_keys, node_page_num);

	//I need the cell number to get the cell, and not the key number. Duh
	//													|
	//													V
	//uint32_t median_cell = *internal_node_cell(node, median_key);

	//Split the original node into two nodes, excluding the middle
	uint32_t new_page_num = get_unused_page_num(table->pager);

	cout << "New Page Num = " << new_page_num << endl;

	void* new_node = get_page(table->pager, new_page_num);

	cout << "Retrieved new node. Now Initializing New Node.\n";

	initialize_internal_node(new_node);

	cout << "Initialized new node. Now updating parent of new node.\n";

	/* Update parent of the new node */
	*node_parent(new_node) = *node_parent(node);

	uint32_t parent_page_num = parent;

	bool is_median = false;

	uint32_t median_page_num;

	cout << "Updated parent. Now copying contents....\n";

	//Copy contents of median + 1 .. max_keys to the new node
	for (uint32_t i = 0; i < INTERNAL_NODE_MAX_CELLS; i++)
	{
		cout << "i = " << i << endl;

		void* destination_node;

		cout << "Created Destination node\n";

		if (i >= INTERNAL_NODE_RIGHT_SPLIT_COUNT)
		{
			cout << "Right Side\n";
			destination_node = new_node;
		}

		else if (i <= INTERNAL_NODE_LEFT_SPLIT_COUNT)
		{
			cout << "Left Side\n";
			destination_node = node;
			cout << "Assigned destination node!\n";
		}

		else
		{
			//Median node must be removed from child to parent
			cout << "Is median\n";
			is_median = true;
			destination_node = get_page(table->pager, parent);
		}

		uint32_t index_within_node;

		if (i > 0 && destination_node == new_node && !is_median)
		{
			index_within_node = i - 1 - INTERNAL_NODE_LEFT_SPLIT_COUNT;
			cout << "Not the median. index_within_node = " << index_within_node << endl;

		}

		else if (destination_node == node && !is_median)
		{
			index_within_node = i;
			cout << "Not the median. index_within_node = " << index_within_node << endl;

		}

		else if (is_median)
		{
			//Median node
		}

		void* destination;

		if(!is_median)
		destination = internal_node_cell(destination_node, index_within_node);

		if (!is_median)
		{
			memcpy(destination, internal_node_cell(node, i), INTERNAL_NODE_CELL_SIZE);
		}

		else
		{
			//Copy median cell to parent
			cout << "Is Median!. Inserting to parent\n";
			uint32_t median_cell = *internal_node_cell(node, i);
			internal_node_insert(table, parent_page_num, median_cell);
			is_median = false;

		}

	}

	//Update cell counts
	*(leaf_node_num_cells(node)) = INTERNAL_NODE_LEFT_SPLIT_COUNT;
	*(leaf_node_num_cells(new_node)) = INTERNAL_NODE_RIGHT_SPLIT_COUNT;

	//Update node's parents
	if (is_root(node))
	{
		create_new_root(table, new_page_num);
	}

	else
	{
		//uint32_t parent_page_num = *node_parent(node);
		uint32_t new_max = get_node_max_key(node);
		void* parent = get_page(table->pager, new_max);

		update_internal_node_key(parent, node_max_keys, new_max);
		internal_node_insert(table, parent_page_num, new_page_num);
		return;
	}

}

void delete_leaf_cell(Table* table, void* node, void* leaf_cell, uint32_t cell_num)
{
	//Deletes a leaf cell from the B Tree
	uint32_t parent_page = *node_parent(node);

	//Case1: No need to join 2 leaf nodes (2 leaf node siblings are atleast half full)

	/* TODO: Implement all cases. Only 2 have been covered as of now */

	if(!(is_root(node)))
	{
		//For Debugging
		//void* prev_cell = leaf_node_cell(node, cell_num - 1);
		//cout << "prev_cell value is " << ((Row*)leaf_node_value(node, cell_num - 1))->id << endl;
		//void* next_cell = leaf_node_cell(node, cell_num + 1);
		//cout << "next_cell value is " << ((Row*)leaf_node_value(node, cell_num + 1))->id << endl;

		if(cell_num == (*(leaf_node_num_cells(node))-1))
		{
			*(leaf_node_num_cells(node)) -= 1;
			return;
		}

		for(uint32_t i= cell_num; i < *(leaf_node_num_cells(node)); i++)
		{
			memcpy(leaf_node_cell(node, i), leaf_node_cell(node, i+1), LEAF_NODE_CELL_SIZE);

			//Now delete the rightmost node
			//free(leaf_node_cell(node, (*leaf_node_num_cells(node)-1)));
		}

		*(leaf_node_num_cells(node)) -= 1;

	}

	else
	{
		cout << "Deleting root....\n";
	}

	table->num_rows -= 1;
	

}

void delete_key(Table* table, uint32_t node_page, uint32_t key_to_delete)
{
	void* node = get_page(table->pager, node_page);

	if(get_node_type(node) == NODE_INTERNAL)
	{
		cout << "Deleting Internal Node....\n";
		uint32_t num_keys = *(internal_node_num_keys(node));
		int32_t pos = -1;
		for(int32_t i = num_keys - 1; i>=0 ; i--)
		{
			if(*(internal_node_key(node, i)) == key_to_delete)
			{
				pos = i;
				break;
			}
		}

		if(pos == -1)
		{
			//Go to its suitable child depending on key comparison
			if(*(internal_node_key(node, num_keys-1)) < key_to_delete)
			{
				//Right child
				uint32_t right_child = *internal_node_right_child(node);
				return delete_key(table, right_child, key_to_delete);
			}

			else
			{
				//Go to left
				int32_t child_num = -1;

				for(uint32_t i=0; i<num_keys; i++)
				{
					if((*internal_node_key(node, i) > key_to_delete))
					{
						return delete_key(table, *internal_node_child(node, i), key_to_delete);
					}
				}

				if(child_num == -1)
				{
					cout << "Node not found.\n";
					exit(EXIT_FAILURE);
				}

				return delete_key(table, *internal_node_child(node, child_num), key_to_delete);
			}			
		}
	}

	else if(get_node_type(node) == NODE_LEAF)
	{
		cout << "Deleting Leaf Node....\n";
		uint32_t num_cells = *(leaf_node_num_cells(node));
		int32_t pos = -1;
		for(int32_t i = num_cells - 1; i >= 0; i--)
		{
			if(*(leaf_node_key(node, i)) == key_to_delete)
			{
				pos = i;
				break;
			} 
		}

		if(pos == -1)
		{
			//Go to next node if you have not reached the end
			cout << "Going to the next node....\n";
			uint32_t next_page = *leaf_node_sibling(node);
			if(next_page == 0)
			{
				cout << "Key to be deleted not found\n";
				exit(EXIT_FAILURE);
			}
			return delete_key(table, next_page, key_to_delete);

		}

		else
		{
			return delete_leaf_cell(table, node, leaf_node_cell(node, pos), pos);
		}
		
	}
}


bool check_duplicate_key(Table* table, uint32_t node_page , uint32_t key_to_insert)
{
	//Start from root, and recurse down the tree
	void* node = get_page(table->pager, node_page);

	uint32_t num_keys = *(internal_node_num_keys(node));

	uint32_t right_page = *(internal_node_right_child(node));


	if(*(internal_node_key(node, *(internal_node_num_keys(node)))) < key_to_insert)
	{
		//Go to right child
		return check_duplicate_key(table, right_page, key_to_insert);
	}

	for(uint32_t i=0; i<num_keys; i++)
	{
		uint32_t key = *(internal_node_key(node, i));
		if(key == key_to_insert)
			return true;

		else if(key_to_insert < key)
		{
			uint32_t child_page = *(internal_node_child(node, i));
			return check_duplicate_key(table, child_page, key_to_insert);
		}
	}

	return false;
}

ExecuteResult execute_insert(Statement* statement, Table* table)
{
	//Executes an insert statement by r/w onto the table
	void* node = get_page(table->pager, table->root_page_num);
	uint32_t num_cells = *(leaf_node_num_cells(node));

	Row* row_to_insert = &(statement->row_to_insert);
	uint32_t key_to_insert = row_to_insert->id;


	//We abstract through using cursors to insert
	//PLAN: Go to end of table and then write to that location
	//We need to allocate new memory for the row to be inserted only at the END of a table

	//We insert the node at the correct position, sorted by Key Value.
	class Cursor* cursor = table_find(table, key_to_insert);

	if (cursor->getCellNum() < num_cells)
	{
		uint32_t key_at_index = *leaf_node_key(node, cursor->getCellNum());
		if (key_at_index == key_to_insert)
			return EXECUTE_DUPLICATE_KEY;
	}

	if(check_key(cursor->getTable()->pager, cursor->getTable()->root_page_num, key_to_insert) == false)
	{
		return EXECUTE_DUPLICATE_KEY;
	}

	leaf_node_insert(cursor, row_to_insert->id, row_to_insert);

	//Free the cursor after writing
	delete cursor;

	return EXECUTE_SUCCESS;
}

ExecuteResult execute_select(Statement* statement, Table* table)
{
	//Executes a select statement by r/w onto the table
	//Implementing basic select operation

	Row row;

	//We now use Cursors for select by moving row wise
	class Cursor* cursor = table_start(table);
	while (!(cursor->getEOT()))
	{
		//Bring data of row from memory
		deserialize_row(cursor_value(cursor), &row);
		print_row(&row);
		cursor_advance(cursor);
	}

	//Free the cursor
	delete cursor;

	return EXECUTE_SUCCESS;

}

ExecuteResult execute_delete(Statement* statement, Table* table)
{
	void* node = get_page(table->pager, table->root_page_num);
	uint32_t num_cells = *(leaf_node_num_cells(node));

	Row* row_to_delete = &(statement->row_to_insert);

	uint32_t key_to_delete = row_to_delete->id;

	if(check_key(table->pager, table->root_page_num, key_to_delete) == true)
	{
		return EXECUTE_TABLE_FULL;
	}

	delete_key(table, table->root_page_num, key_to_delete);

	return EXECUTE_SUCCESS;
}

ExecuteResult execute_statement(Statement* statement, Table* table)
{
	//Executes the statement depending upon the Statement type
	if (statement->type == STATEMENT_SELECT)
	{
		//Execute Select Statement
		ExecuteResult execute_result = execute_select(statement, table);

		return execute_result;
	}

	else if (statement->type == STATEMENT_INSERT)
	{
		//Execute Insert Statement
		ExecuteResult execute_result = execute_insert(statement, table);

		return execute_result;
	}

	else if (statement->type == STATEMENT_DELETE)
	{
		ExecuteResult execute_result = execute_delete(statement, table);

		return execute_result;
	}
}

void print_leaf_node(void* node) {
	uint32_t num_cells = *leaf_node_num_cells(node);
	cout << "(leaf (size " << num_cells << ")" << endl;;
	for (uint32_t i = 0; i < num_cells; i++) {
		uint32_t key = *leaf_node_key(node, i);
		cout << "  - " << i << ": " << key << endl;
	}
}

void print_constants()
{
	cout << "ROW_SIZE: " << ROW_SIZE << endl;
	cout << "COMMON_NODE_HEADER_SIZE: " << COMMON_NODE_HEADER_SIZE << endl;
	cout << "LEAF_NODE_HEADER_SIZE: " << LEAF_NODE_HEADER_SIZE << endl;
	cout << "LEAF_NODE_CELL_SIZE: " << LEAF_NODE_CELL_SIZE << endl;
	cout << "LEAF_NODE_SPACE_FOR_CELLS: " << LEAF_NODE_CELL_SPACE << endl;
	cout << "LEAF_NODE_MAX_CELLS: " << LEAF_NODE_MAX_CELLS << endl;
}

void indent(uint32_t level) 
{
	for (uint32_t i = 0; i < level; i++) 
	{
		cout << "  ";
	}
}

bool check_key(Pager* pager, uint32_t page_num, uint32_t key)
{
  
	void* node = get_page(pager, page_num);
	uint32_t num_keys, child;

	switch (get_node_type(node))
	{
		case (NODE_LEAF):
		num_keys = *leaf_node_num_cells(node);

		for (uint32_t i = 0; i < num_keys; i++) 
		{
			//cout << "Leaf node key is " << *leaf_node_key(node, i) << endl;
			if(*leaf_node_key(node, i) == key)
				return false;
		}
		break;

		case (NODE_INTERNAL):
		num_keys = *internal_node_num_keys(node);

		for (uint32_t i = 0; i < num_keys; i++) 
		{
			child = *internal_node_child(node, i);
			//cout << "child page number is " << child << endl;
			if(check_key(pager, child, key) == false)
				return false;

			//cout << "Internal node key is " << *internal_node_key(node, i) << endl;
			if(*internal_node_key(node, i) == key)
				return false;
		}

		child = *internal_node_right_child(node);
		if(check_key(pager, child, key) == false)
			return false;
		break;
  }

  return true;

}

void print_tree(Pager* pager, uint32_t page_num, uint32_t indentation_level)
{
  
	void* node = get_page(pager, page_num);
	uint32_t num_keys, child;

	switch (get_node_type(node))
	{
		case (NODE_LEAF):
		num_keys = *leaf_node_num_cells(node);
		indent(indentation_level);

		cout << "- leaf (size " << num_keys << ")" << endl;

		for (uint32_t i = 0; i < num_keys; i++) 
		{
			indent(indentation_level + 1);
			cout << "- " << *leaf_node_key(node, i) << endl;
		}
		break;

		case (NODE_INTERNAL):
		num_keys = *internal_node_num_keys(node);
		indent(indentation_level);
		cout << "- internal (size " << num_keys << ")" << endl;

		for (uint32_t i = 0; i < num_keys; i++) 
		{
			child = *internal_node_child(node, i);
			//cout << "child page number is " << child << endl;
			print_tree(pager, child, indentation_level + 1);

			indent(indentation_level + 1);
			cout << "- key " << *internal_node_key(node, i) << endl;
		}

		child = *internal_node_right_child(node);
		print_tree(pager, child, indentation_level + 1);
		break;
  }
}


uint32_t get_count(Pager* pager, uint32_t page_num)
{
  	uint32_t count = 0;
	void* node = get_page(pager, page_num);
	uint32_t num_keys, child;

	switch (get_node_type(node))
	{
		case (NODE_LEAF):
		num_keys = *leaf_node_num_cells(node);
		count += num_keys;
		break;

		case (NODE_INTERNAL):
		num_keys = *internal_node_num_keys(node);
		for (uint32_t i = 0; i < num_keys; i++) 
		{
			child = *internal_node_child(node, i);
			count += get_count(pager, child);
		}

		child = *internal_node_right_child(node);
		count += get_count(pager, child);
		break;
  }
}

void set_root(bool cond)
{
	i_am_root = cond;
}

//Making an REPL loop	

int main(int argc, char* argv[])
{
	//Create a wrapper around the state to be stored, to interact with getline()
	InputBuffer* input_buffer = new_input_buffer();

	//Table* table = new_table();
	if (argc < 2)
	{
		cout << "Must supply a database filename.\n";
		exit(EXIT_FAILURE);
	}

	char* filename = argv[1];
	Table* table = db_open(filename);
    
    bool no_prompt = false;

	//Get the password from the file
	PASS = get_password();

	//Set default user as root - only for testing purposes
	set_root(true);

	while (true)
	{
		print_prompt();
		read_input(input_buffer);

		if (input_buffer->buffer[0] == '.')
		{
			switch (do_meta_command(input_buffer, table))
			{
				//TODO: Exception Handling
			case(META_COMMAND_SUCCESS):
				continue;

			case(META_COMMAND_UNRECOGNIZED):
				cout << "Unrecognized command '" << input_buffer->buffer << "'.\n";
				continue;

			case(META_COMMAND_PASSWORD_FAILURE):
            	cout << "Wrong Password entered too many times. Please try again after " << waiting_time/60 << " minutes.\n";
				potential_attacker = true;
				current_time = time(0);
            	continue;

			case(META_COMMAND_EMPTY):
				continue;

			}
		}

		Statement statement;

		switch (prepare_statement(input_buffer, &statement))
		{
		case (PREPARE_SUCCESS):
			break;

		case (PREPARE_SYNTAX_ERROR):
			cout << "Syntax error. Could not parse statement\n";
			continue;

		case (PREPARE_UNRECOGNIZED_STATEMENT):
			cout << "Unrecognized keyword at start of '" << input_buffer->buffer << "'.\n";
			continue;

		case (PREPARE_USERNAME_OVERFLOW):
			cout << "Error: Username is too long." << endl;
			continue;

		case (PREPARE_EMAIL_OVERFLOW):
			cout << "Error: Email is too long." << endl;
			continue;

		case (PREPARE_NEGAGIVE_ID):
			cout << "Error: ID must be a non-negative Integer." << endl;
			continue;

        case (PREPARE_PASSWORD_FAILURE):
            cout << "Wrong Password entered too many times. Please try again after " << waiting_time/60 << " minutes.\n";
			potential_attacker = true;
			current_time = time(0);
            continue;

		case (PREPARE_EMPTY):
			continue;

		}

		switch (execute_statement(&statement, table))
		{
		case (EXECUTE_SUCCESS):
			cout << "Executed.\n";
			break;

		case (EXECUTE_DUPLICATE_KEY):
			cout << "Error: Duplicate key.\n";
			break;

		case (EXECUTE_TABLE_FULL):
			cout << "Error: Table full.\n";
			break;
		}


	}

}




