#include <algorithm>
#include<cstdio>
#include <fstream>
#include <fcntl.h>
#include <iostream>
#include <limits>
#include <string>
#include <stdbool.h>
#include <sstream>
#include <sys/stat.h>
#include <vector>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#include <io.h>
#define CLOSE(c) _close(c)
#define lseek(fd, pno, size) _lseek(fd, pno, size)
#define SYSTEM 0
#define read(a, b, c) _read(a, b, c)
#define open(a, b, c) _open(a, b, c)
#define write(a, b, c) _write(a, b, c)
#else
#include<unistd.h>
#include<ctime>
#define CLOSE(c) close(c)
#include <limits.h>
#include<string.h>
#define SYSTEM 1
#endif

#if SIZE_MAX == UINT_MAX
typedef int ssize_t;        /* common 32 bit case */
#elif SIZE_MAX == ULONG_MAX
typedef long ssize_t;       /* linux 64 bits */
#elif SIZE_MAX == ULLONG_MAX
typedef long long ssize_t;  /* windows 64 bits */
#elif SIZE_MAX == USHRT_MAX
typedef short ssize_t;      /* is this even possible? */
#else
#error platform has exotic SIZE_MAX
#endif

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 100
#define COLUMN_ADDRESS_SIZE 255
#define COLUMN_AGE_SIZE 6
#define COLUMN_WEIGHT_SIZE 6
#define COLUMN_BLOOD_GROUP_SIZE 6

//Trick for getting the address of the Attribute. sizeof(((Struct*)0)->Attribute) gives size of the attribute
#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)

#define TABLE_MAX_PAGES 100

#define _CRT_SECURE_NO_WARNING

//#define EXIT_FAILURE -1
//#define EXIT_SUCCESS 0

using namespace std;

/*

B-Tree Implementation :

	*Every node has a parent, which has pointers for finding siblings
	*Pages, along with Key Values, are stored in a node in the B-Tree
	*Supply extra pointers (n+1 for a n node B-Tree) to children
	*A Leaf Node has no pointers to

*/

//Enumerations
enum MetaCommandResult_t { META_COMMAND_SUCCESS, META_COMMAND_UNRECOGNIZED, META_COMMAND_PASSWORD_FAILURE, META_COMMAND_EMPTY };
enum PrepareResult_t { PREPARE_SUCCESS, PREPARE_UNRECOGNIZED_STATEMENT, PREPARE_SYNTAX_ERROR, PREPARE_USERNAME_OVERFLOW, PREPARE_EMAIL_OVERFLOW, PREPARE_NEGAGIVE_ID, PREPARE_EMPTY, PREPARE_PASSWORD_SUCCESS, PREPARE_PASSWORD_FAILURE, PREPARE_SELECT_FAILURE, PREPARE_ADDRESS_OVERFLOW, PREPARE_NEGATIVE_AGE, PREPARE_NEGATIVE_WEIGHT, PREPARE_BLOOD_GROUP_OVERFLOW };
enum StatementType_t { STATEMENT_INSERT, STATEMENT_SELECT, STATEMENT_DELETE, STATEMENT_EMPTY };
enum ExecuteResult_t { EXECUTE_SUCCESS, EXECUTE_DUPLICATE_KEY, EXECUTE_TABLE_FULL };
enum NodeType_t { NODE_INTERNAL, NODE_LEAF };
enum NodeLeaf_t { };
enum NodeInternal_t { };

//Structure Definitions
struct InputBuffer_t
{
	//Contains buffer information, and throws error if input_length > buffer_length_MAX
	std::string buffer;
	size_t buffer_length = 0;
	int input_length = 0;
};


struct Row_t
{
	uint32_t id;
	//Allocating extra space due to null terminated C string
	char username[COLUMN_USERNAME_SIZE + 1];
	char email[COLUMN_EMAIL_SIZE + 1];
	char address[COLUMN_ADDRESS_SIZE + 1];
	uint32_t age;
	uint32_t weight;
	char blood_group[COLUMN_BLOOD_GROUP_SIZE + 1];
};

struct Statement_t
{
	//Contains the information about the Statement
	enum StatementType_t type;
	struct Row_t row_to_insert;
};

struct Pager_t
{
	//The Pager is an abstraction that accesses the page when the Table requests a page to be cached/written to disn
	int file_descriptor;
	uint32_t file_length;
	void* pages[TABLE_MAX_PAGES];
	uint32_t num_pages;

};

struct Table_t
{
	uint32_t num_rows;
	//void* pages[TABLE_MAX_PAGES];
	struct Pager_t* pager;
	uint32_t root_page_num;

};


typedef struct InputBuffer_t InputBuffer;
typedef struct MetaCommandList MetaCommandList;
typedef enum MetaCommandResult_t MetaCommandResult;
typedef enum PrepareResult_t PrepareResult;
typedef enum StatementType_t StatementType;
typedef struct Statement_t Statement;
typedef struct Row_t Row;
typedef struct Table_t Table;
typedef enum ExecuteResult_t ExecuteResult;
typedef struct Pager_t Pager;
typedef enum NodeType_t NodeType;


//Const Definitions
const uint32_t ID_SIZE = size_of_attribute(Row, id);
const uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);
const uint32_t ADDRESS_SIZE = size_of_attribute(Row, address);
const uint32_t AGE_SIZE = size_of_attribute(Row, age);
const uint32_t WEIGHT_SIZE = size_of_attribute(Row, weight);
const uint32_t BLOOD_GROUP_SIZE = size_of_attribute(Row, blood_group);


//Create Offsets for accessing via pages
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ADDRESS_OFFSET = EMAIL_OFFSET + EMAIL_SIZE;
const uint32_t AGE_OFFSET = ADDRESS_OFFSET + ADDRESS_SIZE;
const uint32_t WEIGHT_OFFSET = AGE_OFFSET + AGE_SIZE;
const uint32_t BLOOD_GROUP_OFFSET = WEIGHT_OFFSET + WEIGHT_SIZE;

const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE + ADDRESS_SIZE + AGE_SIZE + WEIGHT_SIZE + BLOOD_GROUP_SIZE;

//Ensure Page Size is 4KB as this is the default size for most Computers
const uint32_t PAGE_SIZE = 4096;
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

//Node Properties:
//A Node holds :
//	*Type of Node
//	*Offset associated with it
//	*Whether or not it is a Root Node, and Offset in that case
//	*Pointer to it's parent and offset
//	*A common header - common to all nodes
const uint32_t NODE_TYPE_SIZE = sizeof(uint8_t);
const uint32_t NODE_TYPE_OFFSET = 0;
const uint32_t IS_ROOT_SIZE = sizeof(uint8_t);
const uint32_t IS_ROOT_OFFSET = NODE_TYPE_SIZE;
const uint32_t PARENT_POINTER_SIZE = sizeof(uint32_t);
const uint32_t PARENT_POINTER_OFFSET = IS_ROOT_OFFSET + IS_ROOT_SIZE;
const uint8_t COMMON_NODE_HEADER_SIZE = NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE;

//Leaf Node Properties:
//	*A Header which includes the number of nodes it has, in addition to the Common Header
//	*A field for pointing to it's next sibling leaf node (Here, sibling refers to the NEXT leaf node, to the right)
const uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_SIBLING_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_SIBLING_OFFSET = LEAF_NODE_NUM_CELLS_OFFSET + LEAF_NODE_NUM_CELLS_SIZE;
const uint32_t LEAF_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE + LEAF_NODE_SIBLING_SIZE;
//	*It also consists of a Key-Value pair, which make up the Leaf Node Cell
const uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_VALUE_SIZE = ROW_SIZE;
const uint32_t LEAF_NODE_CELL_SIZE = LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE;
//	*Offsets for the Leaf Node
const uint32_t LEAF_NODE_KEY_OFFSET = 0;
const uint32_t LEAF_NODE_VALUE_OFFSET = LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE;
//const uint32_t LEAF_NODE_CELLS_OFFSET = LEAF_NODE_VALUE_OFFSET;

//	*Since a page is stored in a leaf node, account for the space taken by the headers
//	*Page consists of Leaf Node Data + Leaf Node Cell
const uint32_t LEAF_NODE_CELL_SPACE = PAGE_SIZE - LEAF_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_MAX_CELLS = LEAF_NODE_CELL_SPACE / LEAF_NODE_CELL_SIZE;

//Leaf Node Split Constants for INSERTION (explains the extra +1):
const uint32_t LEAF_NODE_RIGHT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) / 2;
const uint32_t LEAF_NODE_LEFT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) - (LEAF_NODE_RIGHT_SPLIT_COUNT);

//Internal Node Properties:
//	*The common node header
//	*Number of Keys it holds (one more than the number of children)
//	*A pointer to the leftmost key in its right child (in this case, we refer to the page number of the right child)
//	*The extra pointer is stored in the header
const uint32_t INTERNAL_NODE_NUM_KEYS_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_RIGHT_CHILD_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE + INTERNAL_NODE_NUM_KEYS_SIZE + INTERNAL_NODE_RIGHT_CHILD_SIZE;

//The Offsets
const uint32_t INTERNAL_NODE_NUM_KEYS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t INTERNAL_NODE_RIGHT_CHILD_OFFSET = INTERNAL_NODE_NUM_KEYS_OFFSET + INTERNAL_NODE_NUM_KEYS_SIZE;

//Body of the Internal Node
const uint32_t INTERNAL_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_CHILD_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_CELL_SIZE = INTERNAL_NODE_KEY_SIZE + INTERNAL_NODE_CHILD_SIZE;

//Offsets for the keys and children
const uint32_t INTERNAL_NODE_CHILD_OFFSET = INTERNAL_NODE_RIGHT_CHILD_OFFSET + INTERNAL_NODE_RIGHT_CHILD_SIZE;
const uint32_t INTERNAL_NODE_KEY_OFFSET = INTERNAL_NODE_CHILD_OFFSET + INTERNAL_NODE_CHILD_SIZE;

//Max Number of Internal Node Cells
const uint32_t INTERNAL_NODE_MAX_CELLS = 3;

//Split Counts for Internal Nodes
const uint32_t INTERNAL_NODE_RIGHT_SPLIT_COUNT = (INTERNAL_NODE_MAX_CELLS) / 2 + 1;
const uint32_t INTERNAL_NODE_LEFT_SPLIT_COUNT = (INTERNAL_NODE_MAX_CELLS) - (INTERNAL_NODE_RIGHT_SPLIT_COUNT) - 1;

//Password 
#define DEFAULT_PASS 1944140770 //Just Incase
uint32_t PASS = 3604474777;

//Am i currently root or not?
bool i_am_root = false;

//Current Time 
time_t current_time = time(0);
uint32_t waiting_time = 60;

//Potential Attacker
bool potential_attacker = false;

//Select Until
uint32_t select_until_id = 0;

//Class Definitions
class Cursor
{
private:
	//Data member Definitions
	Table* table = NULL;
	//uint32_t row_num = 0;
	uint32_t page_num = 0;
	uint32_t cell_num = 0;
	bool end_of_table = false;

public:
	//Constructor Definitions
	Cursor(Table* table)
	{
		this->table = table;
		//this->end_of_table = (table->num_rows == 0);

		this->cell_num = 0;
		this->page_num = table->root_page_num;

	}

	Cursor(Table* table, int row)
	{
		this->table = table;
		//this->row_num = row;
		//this->end_of_table = (table->num_rows == row);
	}

	//Member Functions
	bool getEOT()
	{
		return end_of_table;
	}

	uint32_t getPage()
	{
		return page_num;
	}

	Table* getTable()
	{
		return table;
	}

	void setRow(int cell)
	{
		if (cell >= 0)
			this->cell_num = cell;
	}

	void increment()
	{
		this->cell_num++;
	}

	void setEOT(bool cond)
	{
		//this->row_num = table->num_rows;
		this->end_of_table = cond;
	}

	void resetEOT()
	{
		this->end_of_table = false;
	}

	void setPageNum(uint32_t page_num)
	{
		this->page_num = page_num;
	}

	void setCellNum(uint32_t num)
	{
		this->cell_num = num;
	}

	uint32_t getCellNum()
	{
		return this->cell_num;
	}

	uint32_t getPageNum()
	{
		return this->page_num;
	}

	~Cursor() {
		//Destructor
	}
};

//Function Prototypes
unsigned hash_str(const char* str);
class Cursor* table_start(Table* table);
void* get_page(Pager* pager, uint32_t page_num);
class Cursor* table_end(Table* table);
class Cursor* table_find(Table* table, uint32_t key);
void* cursor_value(class Cursor* cursor);
void cursor_advance(Cursor* cursor);
InputBuffer* new_input_buffer();
void print_prompt();
istream& my_getline(istream& is, std::string& s, char delim);
uint32_t my_ascii_to_integer(string buffer);
void read_input(InputBuffer* input_buffer);
void close_input_buffer(InputBuffer* input_buffer);
PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement);
void serialize_row(Row* source, void* destination);
void deserialize_row(void* source, Row* destination);
void* get_page(Pager* pager, uint32_t page_num);
void* allocate_row(Table* table, uint32_t row_num);
void print_row(Row* row);
Pager* pager_open(const char* filename);
//void pager_flush(Pager* pager, uint32_t page_num, uint32_t size);
void pager_flush(Pager* pager, uint32_t page_num);
class Cursor* table_find(Table* table, uint32_t key);
Table* db_open(const char* filename);
void db_close(Table* table);
void delete_leaf_cell(Table* table, void* node, void* leaf_cell, uint32_t cell_num);
void delete_key(Table* table, uint32_t node_page, uint32_t key_to_delete);
bool check_duplicate_key(Table* table, uint32_t node_page , uint32_t key_to_insert);
MetaCommandResult do_meta_command(InputBuffer* input_buffer, Table* table);
ExecuteResult execute_insert(Statement* statement, Table* table);
ExecuteResult execute_select(Statement* statement, Table* table);
ExecuteResult execute_statement(Statement* statement, Table* table);
ExecuteResult execute_delete(Statement* statement, Table* table);
uint32_t* node_parent(void* node);
void set_node_type(void* node, NodeType type);
void set_node_root(void* node, bool is_root);
void create_new_root(Table* table, uint32_t right_child_page_num);
NodeType get_node_type(void* node);
uint32_t* leaf_node_num_cells(void* node);
void* leaf_node_cell(void* node, uint32_t cell_num);
uint32_t* leaf_node_key(void* node, uint32_t cell_num);
void* leaf_node_value(void* node, uint32_t cell_num);
uint32_t* internal_node_right_child(void* node);
uint32_t* internal_node_num_keys(void* node);
uint32_t* internal_node_key(void* node, uint32_t key_num);
void internal_node_insert(Table* table, uint32_t parent_page_num, uint32_t child_page_num);
class Cursor* find_internal_node(Table* table, uint32_t page_num, uint32_t key);
uint32_t find_internal_node_child(void* node, uint32_t key);
void initialize_leaf_node(void* node);
void split_leaf_and_insert(class Cursor* cursor, uint32_t key, Row* value);
void split_internal_node(Table* table, void* node, uint32_t node_page_num);
void indent(uint32_t level);
bool check_key(Pager* pager, uint32_t page_num, uint32_t key);
uint32_t get_count(Table* table);
void print_leaf_node(void* node);
void print_tree(Pager* pager, uint32_t page_num, uint32_t indentation_level);
void print_constants();
