#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

// --------------------------------------------------------------------
// STACK
// --------------------------------------------------------------------

typedef struct stack_node {
    void* data;
    struct stack_node* prev;
} stack_node;


stack_node* create_stack_node(const void* data, size_t size_of_data) {
    if (!data || size_of_data == 0)
        return NULL;

    stack_node* new_node = (stack_node*) malloc(sizeof(stack_node));
    if (!new_node)
        return NULL;

    new_node->data = malloc(size_of_data);
    if (!new_node->data) {
        free(new_node);
        return NULL;
    }

    memcpy(new_node->data, data, size_of_data);
    new_node->prev = NULL;

    return new_node;
}

// frees node without care about the previous
void hard_free_stack_node(stack_node* node) {
    if (!node)
        return;

    free(node->data);
    free(node);
}

// returns new head pointer or NULL
stack_node* pop_stack_node(stack_node* head) {
    if (!head)
        return NULL;

    stack_node* prev = head->prev;
    hard_free_stack_node(head);
    return prev;
}

// returns top data pointer or NULL
const void* top_stack_data(const stack_node* phead) {
    if (!phead)
        return NULL;

    return phead->data;
}

const void* prev_stack_data(const stack_node* phead) {
    if (!phead || !phead->prev)
        return NULL;
    return phead->prev->data;
}

// returns new head pointer or current head pointer in the case of failure
stack_node* push_data(stack_node* head, const void* data, size_t size_of_data) {
    stack_node* new_head = create_stack_node(data, size_of_data);
    if (!new_head)
        return head;

    new_head->prev = head;
    return new_head;
}

void clear_stack(stack_node** head) {
    if (!head)
        return;

    while ((*head = pop_stack_node(*head)));
    *head = NULL;
}

// --------------------------------------------------------------------
// READ UNKNOWN LENGTH LINE
// --------------------------------------------------------------------

char* grow_line_buffer(char* line, size_t* cur_buffer_size) {
    if (!line || !cur_buffer_size)
        return NULL;
    *cur_buffer_size *= 2;
    char* temp_line = (char*) realloc(line, *cur_buffer_size);
    if (!temp_line) {
        free(line);
        return NULL;
    }
    return temp_line;
}

char* read_line() {
    ptrdiff_t cur_len = 0;
    size_t cur_buffer = 11;

    char* line = (char*) malloc(cur_buffer);
    if (!line)
        return NULL;

    line[0] = '\0';
    for (char* cur_line_ptr = line; fgets(cur_line_ptr, (int)(cur_buffer - cur_len), stdin); cur_line_ptr = line + cur_len) {
        cur_len += strlen(cur_line_ptr);
        if (line[cur_len - 1] == '\n') {
            line[cur_len - 1] = '\0';
            return line;
        }

        line = grow_line_buffer(line, &cur_buffer);
        if (!line)
            return NULL;
    }

    if (strlen(line) == 0) {
        free(line);
        line = NULL;
    }

    return line;
}

void free_line(char** line) {
    if (!line)
        return;
    free(*line);
    *line = NULL;
}

// --------------------------------------------------------------------
// DETECTORS
// --------------------------------------------------------------------

// * / + - ( )
int is_operator(char symbol) {
    return symbol == '*' || symbol == '/' ||
           symbol == '+' || symbol == '-' ||
           symbol == '(' || symbol == ')';
}

// [0; 9] and '.'
int is_part_of_number(char symbol) {
    return (symbol >= '0' && symbol <= '9') || symbol == '.';
}

// returns EXIT_FAILURE in the case of division by zero or null pointer of result
int process_operation(double *result, double left_operand, double right_operand, char op) {
    if (!result)
        return EXIT_FAILURE;

    if (op == '+') {
        *result = left_operand + right_operand;
        return EXIT_SUCCESS;
    }

    if (op == '-') {
        *result = left_operand - right_operand;
        return EXIT_SUCCESS;
    }

    if (op == '*') {
        *result = left_operand * right_operand;
        return EXIT_SUCCESS;
    }

    if (op == '/' && right_operand != 0.000) {
        *result = left_operand / right_operand;
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
};

// in the case of failure returns negative value
int get_priority(char symbol) {
    if (symbol == '-' || symbol == '+')
        return 2;
    if (symbol == '*' || symbol == '/')
        return 1;
    if (symbol == '(')
        return 3;
    if (symbol == ')')
        return 3;

    return -10;
}

const char* skip_spaces(const char* line) {
    if (line == NULL)
        return NULL;

    while (*line == ' ')
        line++;

    return line;
}

// --------------------------------------------------------------------
// CONVERTING TO DOUBLE
// --------------------------------------------------------------------

int convert_interval_to_double_step(double* dest, char symbol, double* pow, int* is_point_was) {
    if (symbol == '.') {
        if (*is_point_was)
            return 0;

        *is_point_was = 1;
        *pow = 0.100;
        return 1;
    }

    double cur_digit = symbol - '0';

    if (*is_point_was) {
        *dest += cur_digit * *pow;
        *pow /= 10.000;
    }
    else {
        *dest = *dest * 10.000 + cur_digit;
        *pow *= 10.000;
    }

    return 1;
}

const char* convert_interval_to_double(const char* line, double* result) {
    if (!line || !result)
        return NULL;

    *result = 0.000;
    double pow = 1.0;
    int is_point_was = 0;
    line = skip_spaces(line);
    while (is_part_of_number(*line)) {
        if (!convert_interval_to_double_step(result, *line, &pow, &is_point_was))
            return NULL;

        line++;
        line = skip_spaces(line);
    }
    return line;
}

// --------------------------------------------------------------------
// LINE PARSING
// --------------------------------------------------------------------

const char* process_operator_insertion(stack_node** operators, const char* line) {
    stack_node* temp_operators = push_data(*operators, line, sizeof(char));
    if (temp_operators == *operators)
        return NULL;
    *operators = temp_operators;
    return ++line;
}

const char* process_number_insertion(stack_node** numbers, const char* line, double sign) {
    double result = 0.0;
    line = convert_interval_to_double(line, &result);
    result *= sign;
    stack_node* temp_numbers = push_data(*numbers, &result, sizeof(double));
    if (temp_numbers == *numbers)
        return NULL;
    *numbers = temp_numbers;

    return line;
}

// line[0] should be '-'
const char* process_minus_or_number_insertion
        (stack_node** numbers, stack_node** operators, const char* line, int* last_is_number) {

    if (*line != '-')
        return NULL;

    const char* next_pos = skip_spaces(line + 1);

    if (is_operator(*next_pos) || (is_part_of_number(*next_pos) && *last_is_number)) {
        *last_is_number = 0;
        if (process_operator_insertion(operators, line) == NULL)
            return NULL;
        return next_pos;
    }

    if (is_part_of_number(*next_pos)) {
        *last_is_number = 1;
        return process_number_insertion(numbers, next_pos, -1.000);
    }

    return NULL;
}

const char* add_next_to_stacks
        (stack_node** numbers, stack_node** operators, const char* line, int* last_is_number) {

    if (!numbers || !operators || !line || !last_is_number)
        return NULL;

    line = skip_spaces(line);

    if (is_operator(*line) && *line != '-') {
        *last_is_number = 0;
        return process_operator_insertion(operators, line);
    }

    if (is_part_of_number(*line)) {
        *last_is_number = 1;
        return process_number_insertion(numbers, line, 1.000);
    }

    if (*line == '-')
        return process_minus_or_number_insertion(numbers, operators, line, last_is_number);

    return NULL;
}

// --------------------------------------------------------------------
// EVALUATION OF EXPRESSION
// --------------------------------------------------------------------

typedef enum EOE_POP {
    EOE_BOTH_POPPED,
    EOE_LAST_POPPED,
    EOE_NO_ONE_POPPED
} EOE_POP;

EOE_POP top_operator(stack_node** operators, char* result) {
    char* temp = (char*)top_stack_data(*operators);
    if (!temp)
        return EOE_NO_ONE_POPPED;

    *result = *temp;
    *operators = pop_stack_node(*operators);
    return EOE_LAST_POPPED;
}

EOE_POP top_numbers(stack_node** numbers, double* prev, double* top) {
    double* temp_top = (double*)top_stack_data(*numbers);
    if (!temp_top)
        return EOE_NO_ONE_POPPED;
    *top = *temp_top;
    *numbers = pop_stack_node(*numbers);

    double* temp_prev = (double*)top_stack_data(*numbers);
    if (!temp_prev)
        return EOE_LAST_POPPED;
    *prev = *temp_prev;
    *numbers = pop_stack_node(*numbers);

    return EOE_BOTH_POPPED;
}

int do_arithmetic_step(stack_node** numbers, stack_node** operators, char* prev_op) {
    double top_num = 0.000;
    double prev_num = 0.000;
    double result_num = 0.0;
    if (top_numbers(numbers, &prev_num, &top_num) != EOE_BOTH_POPPED)
        return 0;

    if (process_operation(&result_num, prev_num, top_num, *prev_op) == EXIT_FAILURE)
        return 0;

    *numbers = push_data(*numbers, &result_num, sizeof(double));

    if (top_operator(operators, prev_op) == EOE_NO_ONE_POPPED)
        return 0;

    return 1;
}

void compute_opening_br_case(stack_node** numbers, stack_node** operators) {
    char prev_op = ' ';
    if (top_operator(operators, &prev_op) == EOE_NO_ONE_POPPED)
        return;

    while (*operators && prev_op != '(') {
        if(!do_arithmetic_step(numbers, operators, &prev_op))
            return;
    }
}

void compute_priority_case(stack_node** numbers, stack_node** operators, char top_op) {
    char prev_op = ' ';
    if (top_operator(operators, &prev_op) == EOE_NO_ONE_POPPED)
        return;

    while (*operators && get_priority(prev_op) <= get_priority(top_op)) {
        if(!do_arithmetic_step(numbers, operators, &prev_op))
            return;
    }

    *operators = push_data(*operators, &prev_op, sizeof(char));
}

void compute_stack_step(stack_node** numbers, stack_node** operators) {
    char top_op = ' ';

    if (top_operator(operators, &top_op) == EOE_NO_ONE_POPPED)
        return;

    if (top_op == '(') {
        *operators = push_data(*operators, &top_op, sizeof(char));
        return;
    }

    if (top_op == ')') {
        compute_opening_br_case(numbers, operators);
        return;
    }

    compute_priority_case(numbers, operators, top_op);
    *operators = push_data(*operators, &top_op, sizeof(char));
}

// --------------------------------------------------------------------
// HEAD
// --------------------------------------------------------------------

int compute_line(const char* line, double *result) {
    if (!line || !result)
        return EXIT_FAILURE;

    stack_node* numbers = NULL;
    stack_node* operators = NULL;
    int last_is_number = 0;
    char opening_b = '(';
    char closing_b = ')';
    operators = push_data(operators, &opening_b, sizeof(char));

    while (line && *line) {
        line = add_next_to_stacks(&numbers, &operators, line, &last_is_number);
        compute_stack_step(&numbers, &operators);
    }

    operators = push_data(operators, &closing_b, sizeof(char));
    compute_stack_step(&numbers, &operators);

    if (!line || operators || !top_stack_data(numbers) || prev_stack_data(numbers)) {
        clear_stack(&numbers);
        clear_stack(&operators);
        return EXIT_FAILURE;
    }

    *result = *(double*)top_stack_data(numbers);
    clear_stack(&numbers);
    clear_stack(&operators);
    return EXIT_SUCCESS;
}

// --------------------------------------------------------------------
// MAIN
// --------------------------------------------------------------------

int main() {

    char* line = read_line();
    if (!line) {
        printf("[error]\n");
        return 0;
    }

    double result = 0.0;

    if (compute_line(line, &result) == EXIT_FAILURE) {
        free_line(&line);
        printf("[error]\n");
        return 0;
    }
    printf("%.*lf\n", 2, result);
    free_line(&line);

    return 0;
}