// qm_minimizer.c
// Build: gcc -O2 -std=c11 qm_minimizer.c -o qm_minimizer

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>

/* ---- your original stack/node types + helpers ---- */
typedef struct node {
    char data;
    struct node* next;
} node;

typedef struct stack {
    node* head;
} stack;

typedef struct bool_node {
    char data;
    int val;
} bnode;

typedef enum {
    SOP,
    POS
} Mode;

typedef char* (*PatternToTermFunc)(const char*, stack*);

node* newNode(char data) {
    node* temp = (node*)malloc(sizeof(node));
    temp->data = data;
    temp->next = NULL;
    return temp;
}

bnode* newBoolNode(char data, int val) {
    bnode* temp = (bnode*)malloc(sizeof(bnode));
    temp->data = data;
    temp->val = val;
    return temp;
}

stack* newStack() {
    stack* temp = (stack*)malloc(sizeof(stack));
    temp->head = NULL;
    return temp;
}

bool isEmpty(stack* st) {
    return (st->head == NULL);
}

void push(stack* st, char val) {
    node* temp = newNode(val);
    temp->next = st->head;
    st->head = temp;
}

char pop(stack* st) {
    if(isEmpty(st)) {
        // Underflow
        return '\0';
    }
    node* temp = st->head;
    char rt = temp->data;
    st->head = st->head->next;
    free(temp);
    return rt;
}

char peek(stack* st) {
    if(isEmpty(st)) return '\0';
    return st->head->data;
}

void free_stack(stack* st) {
    if (st == NULL) return;
    node* current = st->head;
    node* temp;
    while (current != NULL) {
        temp = current->next;
        free(current);
        current = temp;
    }
    free(st);
}

void reverseStack(stack* st) {
    node *prev = NULL, *current = st->head, *next = NULL;
    while (current != NULL) {
        next = current->next;
        current->next = prev;
        prev = current;
        current = next;
    }
    st->head = prev;
}

/* ---- some helper functions from your code ---- */

char* addImplicitANDs(char* expr) {
    int len = strlen(expr);
    char* newExpr = (char*)malloc(2*len + 1);
    int j = 0;
    for(int i = 0; i < len; i++) {
        char ch = expr[i];
        newExpr[j++] = ch;
        if (i < len - 1) {
            char next = expr[i+1];
            bool curr_is_operand = isalpha(ch) || ch == ')'|| ch == '\'';
            bool next_is_operand = isalpha(next) || next == '(';
            if(curr_is_operand && next_is_operand) {
                newExpr[j++] = '.';
            }
        }
    }
    newExpr[j] = '\0';
    return newExpr;
}

// To check if the expression has valid parentheses
bool hasBalancedParentheses(char *expr) {
    int len = strlen(expr);
    stack *st = newStack();
    for(int i=0; i<len; i++) {
        char ch = expr[i];
        if(ch == '(') push(st,ch);
        else if (ch == ')'){
            if (isEmpty(st)) { free_stack(st); return false; }
            char topchar = pop(st);
            if(topchar != '(') { free_stack(st); return false; }
        }
    }
    bool balanced = isEmpty(st);
    free_stack(st);
    return balanced;
}


// This is to check if the expressions has valid characters or not
bool hasValidCharacters(char* expr) {
    int len = strlen(expr);
    for(int i=0; i<len; i++) {
        char ch = expr[i];
        if ((!isalpha(ch) && ch != '+' && ch != '.' && ch != '(' && ch != ')' && ch != '\'')) 
            return false;
    }
    return true;
}

bool hasValidOperatorPlacement(char* expr) {
    int len = strlen(expr);
    if(!len) return true;
    if (expr[0] == '+' || expr[0] == '.' || expr[len-1] == '+' || expr[len-1] == '.' || expr[0] == '\'') 
        return false;
    for (int i = 0; i < len; i++) {
        char current = expr[i];
        char prev = (i > 0) ? expr[i-1] : '\0';
        char next = (i + 1 < len) ? expr[i+1] : '\0';
        if (current == '+' || current == '.') {
            if (prev == '(' || prev == '+' || prev == '.') return false;
            if (next == ')' || next == '+' || next == '.') return false;
        }
        else if (current == '\'') {
            if (!isalpha(prev) && prev != ')') return false;
        }
        else if (current == '(' && next == ')') {
            return false;
        }
    }
    return true;
}

bool isValidExpression(char* expr) {
    if (!hasValidCharacters(expr)) {
        //printf("Validation Failed: Expression contains invalid characters.\n");
        return false;
    }
    if (!hasBalancedParentheses(expr)) {
        //printf("Validation Failed: Parentheses are not balanced.\n");
        return false;
    }
    if (!hasValidOperatorPlacement(expr)) {
        //printf("Validation Failed: Invalid placement of operators.\n");
        return false;
    }
    return true;
}

/* precedence for infix->postfix */
int pre(char ch) {
    if(ch=='+') return 1;
    if(ch=='.') return 2;
    if(ch=='\'') return 3;
    return 0;
}

/* infix to postfix (push characters onto st as postfix result) */
void inf_to_posf(char* expr, stack* st) {
    stack* op = newStack();
    for(int i = 0; i < strlen(expr); i++) {
        char ch = expr[i];
        if(isalpha(ch)) push(st,ch);
        else if(ch=='(') push(op,ch);
        else if(ch==')') {
            while(!isEmpty(op) && peek(op)!='(') push(st,pop(op));
            if(!isEmpty(op)) pop(op);
        }
        else {
            while(!isEmpty(op) && pre(ch)<=pre(peek(op)) && peek(op)!= '(') push(st,pop(op));
            push(op,ch);
        }
    }
    while(!isEmpty(op)) push(st, pop(op));
    free_stack(op);
}

/* insertion sort on single char stack (used for distinct var ordering) */
void sortedInsert(stack* st, char val) {
    if (isEmpty(st) || val > peek(st)) {
        push(st, val);
        return;
    }
    char temp = pop(st);
    sortedInsert(st, val);
    push(st, temp);
}

void insertionSort(stack* st) {
    if (!isEmpty(st)) {
        char temp = pop(st);
        insertionSort(st);
        sortedInsert(st, temp);
    }
}

/* get distinct variables from postfix stack */
stack* getDistinctVar(stack* st) {
    stack* var = newStack();
    node* itr1 = st->head;
    while(itr1) {
        bool found = false;
        if(isalpha(itr1->data)) {
            if(isEmpty(var)) push(var,itr1->data);
            else {
                node* itr2 = var->head;
                while(itr2) {
                    if(itr1->data==itr2->data) { found = true; break; }
                    itr2=itr2->next;
                }
                if(!found) push(var,itr1->data);
            }
        }
        itr1=itr1->next;
    }
    insertionSort(var);
    return var;
}

void string_to_uppercase(char *str) {
    if (str == NULL) return;
    for (int i = 0; str[i] != '\0'; i++) {
        str[i] = toupper(str[i]);
    }
}

/**
 * Checks if a character 'c' is already present in a string 'arr'.
 */
bool is_char_in_string(char c, const char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == c) {
            return true;
        }
    }
    return false;
}

/**
 * Sorts a character string in place (simple bubble sort).
 */
void sort_string_alphabetically(char *str) {
    int n = strlen(str);
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (str[j] > str[j + 1]) {
                // Swap characters
                char temp = str[j];
                str[j] = str[j + 1];
                str[j + 1] = temp;
            }
        }
    }
}

int parse_variable_list(const char *raw_input, char *unique_vars_out) {
    int var_count = 0;
    
    // Iterate through the entire raw input string
    for (int i = 0; raw_input[i] != '\0'; i++) {
        
        char c = raw_input[i];

        // 1. We only care about letters
        if (isalpha(c)) {
            
            // 2. Standardize to uppercase
            c = toupper(c);

            // 3. Check for duplicates before adding
            if (!is_char_in_string(c, unique_vars_out)) {
                // Safety check: Don't overflow (max 26 letters)
                if (var_count < 26) { 
                    unique_vars_out[var_count] = c;
                    var_count++;
                }
            }
        }
        // If it's a comma, space, or anything else, we just ignore it!
    }

    // Null-terminate the new string
    unique_vars_out[var_count] = '\0';

    // 4. Sort the final string for consistent output (e.g., "ABC" not "BCA")
    sort_string_alphabetically(unique_vars_out);

    return var_count; // Return the count of unique variables
}

stack* getVarStackFromCleanString(const char* clean_str) {
    stack* var_stack = newStack(); // Assumes you have a create_stack()
    int len = strlen(clean_str);
    
    // Push in reverse order
    for (int i = len - 1; i >= 0; i--) {
        // Assumes you have a push()
        push(var_stack, clean_str[i]); 
    }
    
    // The stack will be: head -> 'A' -> 'B' -> 'C' -> NULL
    return var_stack;
}

int* decimal_to_binary(int num, int bits) {
    int* binary = (int*)calloc(bits,sizeof(int));
    int i = bits-1;
    while(num>0 && i>=0) {
        binary[i]=num%2;
        num=num/2;
        i--;
    }
    return binary;
}

/* evaluate postfix boolean where variables already substituted as '0' or '1' chars */
int evaluate_psf(char* expr) {
    stack* st = newStack();
    int len = strlen(expr);
    for (int i = 0; i < len; i++) {
        char ch = expr[i];
        if (ch == '1' || ch == '0') push(st, ch);
        else if (ch == '\'') {
            char op = pop(st);
            int val = op - '0';
            int result = !val;
            push(st, result + '0');
        }
        else if (ch == '+' || ch == '.') {
            char op2 = pop(st);
            char op1 = pop(st);
            int val1 = op1 - '0';
            int val2 = op2 - '0';
            int result;
            if (ch == '+') result = val1 || val2;
            else result = val1 && val2;
            push(st, result + '0');
        }
    }
    int final = pop(st) - '0';
    free_stack(st);
    return final;
}

/* Build minterm table from postfix stack (same logic you had) */
bnode** getMinterms(stack* st) {
    reverseStack(st);
    int st_length = 0;
    node* itr = st->head;
    while(itr) { st_length++; itr = itr->next; }
    char* psf_expr = (char*)calloc((st_length+1),sizeof(char));
    itr = st->head;
    int k = 0;
    stack* var = getDistinctVar(st);
    reverseStack(var);
    itr = var->head;
    int length=0;
    while(itr) { length++; itr=itr->next; }
    int rows = pow(2,length);
    int cols = length+1;
    bnode** minterms = (bnode**)malloc(rows*sizeof(bnode*));
    for(int i = 0; i<rows; i++) minterms[i] = (bnode*)malloc(cols*sizeof(bnode));
    for(int i = 0; i < rows ; i++) {
        itr = var->head;
        int* binary = decimal_to_binary(i, length);
        int j=0;
        while(itr) {
            minterms[i][j].data = itr->data;
            minterms[i][j].val = binary[j];
            itr= itr->next;
            j++;
        }
        free(binary);
    }
    for(int i = 0; i < rows ; i++) {
        itr = st->head;
        k = 0;
        while (itr && k < st_length) {
            psf_expr[k] = itr->data;
            itr = itr->next;
            k++;
        }
        psf_expr[st_length] = '\0';
        for(int j = 0 ; j < cols-1 ; j++) {
            itr = st->head;
            k = 0;
            while(itr && k < st_length) {
                if(itr->data == minterms[i][j].data) {
                    psf_expr[k] = (char)(minterms[i][j].val + '0');
                }
                itr = itr->next;
                k++;
            }
        }
        minterms[i][length].val = evaluate_psf(psf_expr);
    }
    free(psf_expr);
    free_stack(var);
    return minterms;
}

/* ---- Quine-McCluskey implementation ---- */

typedef struct {
    char *pattern;        // e.g. "10-1"
    int *covered;         // list of original minterms covered
    int covered_count;
    bool combined;        // flag when it's combined into higher grouping
} implicant_t;

/* check whether two patterns differ in exactly one position (ignoring '-') and produce combined pattern */
bool combine_patterns(const char *a, const char *b, char *out) {
    int n = strlen(a);
    int diffs = 0;
    for (int i = 0; i < n; ++i) {
        if (a[i] == b[i]) out[i] = a[i];
        else {
            // if either is '-', cannot combine if the other differs (only combine when they differ 0/1 in single pos)
            if (a[i] == '-' || b[i] == '-') return false;
            diffs++;
            out[i] = '-';
            if (diffs > 1) return false;
        }
    }
    out[n] = '\0';
    return diffs == 1;
}

/* check if implicant pattern covers the minterm (binary representation) */
bool pattern_covers(const char *pat, int *binary, int bits) {
    for (int i = 0; i < bits; ++i) {
        if (pat[i] == '-') continue;
        if ((pat[i] - '0') != binary[i]) return false;
    }
    return true;
}

/* convert integer minterm to its binary string into an int array */
int* minterm_to_binary(int m, int bits) {
    return decimal_to_binary(m, bits);
}

/* add new implicant to dynamic array */
void push_implicant(implicant_t **arr, int *cnt, int *cap, const char *pattern, int *covered, int covered_count) {
    if (*cnt >= *cap) {
        *cap = (*cap == 0) ? 8 : (*cap * 2);
        *arr = (implicant_t*)realloc(*arr, (*cap) * sizeof(implicant_t));
    }
    implicant_t *imp = &((*arr)[(*cnt)++]);
    imp->pattern = strdup(pattern);
    imp->covered_count = covered_count;
    imp->covered = (int*)malloc(sizeof(int)*covered_count);
    memcpy(imp->covered, covered, sizeof(int)*covered_count);
    imp->combined = false;
}

/* check if two covered lists represent the same implicant (same set) */
bool same_covered(int *a, int na, int *b, int nb) {
    if (na != nb) return false;
    for (int i = 0; i < na; ++i) if (a[i] != b[i]) return false;
    return true;
}

/* free implicant array */
void free_implicants(implicant_t *arr, int cnt) {
    for (int i = 0; i < cnt; ++i) {
        free(arr[i].pattern);
        free(arr[i].covered);
    }
    free(arr);
}

/* remove duplicate implicants (same pattern) */
void unique_implicants(implicant_t **arr, int *cnt) {
    if (*cnt <= 1) return;
    for (int i = 0; i < *cnt; ++i) {
        for (int j = i+1; j < *cnt; ) {
            if (strcmp((*arr)[i].pattern, (*arr)[j].pattern) == 0) {
                // remove j by shifting
                free((*arr)[j].pattern);
                free((*arr)[j].covered);
                for (int k = j; k < *cnt-1; ++k) (*arr)[k] = (*arr)[k+1];
                (*cnt)--;
            } else j++;
        }
    }
}

/* count literals (number of non '-' ) in pattern */
int literal_count(const char *pat) {
    int c = 0;
    for (int i = 0; pat[i]; ++i) if (pat[i] != '-') c++;
    return c;
}

/* main QM function: input minterm integers, bits; output chosen implicant patterns in result_patterns (malloc'd), result_count */
void quine_mccluskey(int *minterms, int minterm_count, int bits, char ***result_patterns, int *result_count) {
    *result_patterns = NULL;
    *result_count = 0;

    if (minterm_count == 0) {
        // function is 0
        return;
    }
    if (minterm_count == (1 << bits)) {
        // function is 1
        *result_patterns = (char**)malloc(sizeof(char*));
        *result_patterns[0] = strdup("1"); // special marker
        *result_count = 1;
        return;
    }

    /* initial implicants: pattern = binary string, covered = [minterm] */
    implicant_t *current = NULL;
    int cur_cnt = 0, cur_cap = 0;
    for (int i = 0; i < minterm_count; ++i) {
        char *pat = (char*)malloc(bits+1);
        int *bin = minterm_to_binary(minterms[i], bits);
        for (int b = 0; b < bits; ++b) pat[b] = bin[b] + '0';
        pat[bits] = '\0';
        int cov = minterms[i];
        int covs[1]; covs[0] = cov;
        push_implicant(&current, &cur_cnt, &cur_cap, pat, covs, 1);
        free(bin);
        free(pat);
    }

    /* iterative combination */
    implicant_t *primes = NULL;
    int primes_cnt = 0, primes_cap = 0;

    while (true) {
        // group by number of ones (not necessary to store groups explicitly here)
        implicant_t *next = NULL;
        int next_cnt = 0, next_cap = 0;
        // flags to mark combined
        for (int i = 0; i < cur_cnt; ++i) current[i].combined = false;

        for (int i = 0; i < cur_cnt; ++i) {
            for (int j = i+1; j < cur_cnt; ++j) {
                int n = bits;
                char *combined = (char*)malloc(n+1);
                if (combine_patterns(current[i].pattern, current[j].pattern, combined)) {
                    // new pattern; combine covered minterms (union, sorted, unique)
                    // merge two sorted covered arrays (they are single elem initially but may grow)
                    int a_cnt = current[i].covered_count;
                    int b_cnt = current[j].covered_count;
                    int *merged = (int*)malloc(sizeof(int)*(a_cnt + b_cnt));
                    int p=0, q=0, r=0;
                    // covered arrays are not guaranteed sorted; let's create simple union ensuring uniqueness
                    // simpler: copy both then sort & unique
                    for (int t=0;t<a_cnt;++t) merged[r++]=current[i].covered[t];
                    for (int t=0;t<b_cnt;++t) merged[r++]=current[j].covered[t];
                    // sort merged small array
                    for (int x=0;x<r-1;++x) for (int y=x+1;y<r;++y) if (merged[x] > merged[y]) { int tmp=merged[x]; merged[x]=merged[y]; merged[y]=tmp; }
                    int unique_cnt = 0;
                    for (int x=0;x<r;++x) if (x==0 || merged[x]!=merged[x-1]) merged[unique_cnt++] = merged[x];
                    // check if combined pattern already exists in next (same covered set)
                    bool exists = false;
                    for (int z = 0; z < next_cnt; ++z) {
                        if (strcmp(next[z].pattern, combined) == 0) { exists = true; break; }
                    }
                    if (!exists) {
                        push_implicant(&next, &next_cnt, &next_cap, combined, merged, unique_cnt);
                    } else {
                        free(merged);
                    }
                    free(combined);
                    current[i].combined = true;
                    current[j].combined = true;
                } else {
                    free(combined);
                }
            }
        }

        // collect those that were not combined -> prime implicants
        for (int i = 0; i < cur_cnt; ++i) {
            if (!current[i].combined) {
                // add to primes if not duplicate
                bool dup = false;
                for (int p = 0; p < primes_cnt; ++p) {
                    if (strcmp(primes[p].pattern, current[i].pattern) == 0) { dup = true; break; }
                }
                if (!dup) {
                    // add a copy
                    push_implicant(&primes, &primes_cnt, &primes_cap, current[i].pattern, current[i].covered, current[i].covered_count);
                }
            }
        }

        // if no next (no combinations made) break
        if (next_cnt == 0) {
            // finished
            free_implicants(current, cur_cnt);
            break;
        } else {
            // continue with next
            free_implicants(current, cur_cnt);
            current = next;
            cur_cnt = next_cnt;
            cur_cap = next_cap;
            next = NULL;
            next_cnt = 0;
            next_cap = 0;
        }
    }

    /* primes now has all prime implicants */
    // Build prime implicant chart: rows primes_cnt, cols minterm_count
    if (primes_cnt == 0) {
        // unexpected, but return empty
        *result_count = 0;
        return;
    }

    // Map minterm value -> column index
    int *minterm_vals = (int*)malloc(sizeof(int)*minterm_count);
    for (int i = 0; i < minterm_count; ++i) minterm_vals[i] = minterms[i];

    // For each prime implicant, build coverage list of columns
    int **chart = (int**)malloc(sizeof(int*)*primes_cnt);
    for (int p = 0; p < primes_cnt; ++p) {
        chart[p] = (int*)calloc(minterm_count, sizeof(int));
        for (int m = 0; m < minterm_count; ++m) {
            // check if primes[p] covers minterm m
            int *bin = minterm_to_binary(minterm_vals[m], bits);
            if (pattern_covers(primes[p].pattern, bin, bits)) chart[p][m] = 1;
            free(bin);
        }
    }

    // Find essential prime implicants (columns covered by exactly one prime)
    bool *col_covered = (bool*)calloc(minterm_count, sizeof(bool));
    bool *prime_selected = (bool*)calloc(primes_cnt, sizeof(bool));
    for (int m = 0; m < minterm_count; ++m) {
        int coverers = 0;
        int last = -1;
        for (int p = 0; p < primes_cnt; ++p) if (chart[p][m]) { coverers++; last = p; }
        if (coverers == 1) {
            prime_selected[last] = true;
            // mark all columns covered by this prime as covered
            for (int mm = 0; mm < minterm_count; ++mm) if (chart[last][mm]) col_covered[mm] = true;
        }
    }

    // collect minterms still uncovered
    int remaining = 0;
    for (int m = 0; m < minterm_count; ++m) if (!col_covered[m]) remaining++;

    // If remaining > 0, apply Petrick's method to choose minimal set of primes that cover remaining columns
    // For each remaining minterm create a list of primes that cover it.
    // Then compute all combinations (product of sums) and pick minimal by (1) fewest primes, (2) fewest literals.
    if (remaining > 0) {
        // Build list-of-lists
        int **lists = (int**)malloc(sizeof(int*)*remaining);
        int *lists_len = (int*)malloc(sizeof(int)*remaining);
        int idx=0;
        for (int m = 0; m < minterm_count; ++m) if (!col_covered[m]) {
            // gather primes covering m
            int cnt = 0;
            for (int p = 0; p < primes_cnt; ++p) if (chart[p][m]) cnt++;
            lists_len[idx] = cnt;
            lists[idx] = (int*)malloc(sizeof(int)*cnt);
            int t=0;
            for (int p = 0; p < primes_cnt; ++p) if (chart[p][m]) lists[idx][t++]=p;
            idx++;
        }
        // initial combos: singletons from first list
        // Each combo represented as dynamic array of primes (unique)
        typedef struct combo {
            int *pr;
            int len;
        } combo_t;

        combo_t *combos = NULL;
        int combos_cnt = 0, combos_cap = 0;
        // seed
        for (int i = 0; i < lists_len[0]; ++i) {
            int p = lists[0][i];
            combo_t c;
            c.len = 1;
            c.pr = (int*)malloc(sizeof(int));
            c.pr[0] = p;
            if (combos_cnt >= combos_cap) {
                combos_cap = (combos_cap==0)?8:combos_cap*2;
                combos = (combo_t*)realloc(combos, sizeof(combo_t)*combos_cap);
            }
            combos[combos_cnt++] = c;
        }
        // multiply with remaining lists
        for (int L = 1; L < remaining; ++L) {
            combo_t *newc = NULL;
            int newc_cnt = 0, newc_cap = 0;
            for (int i = 0; i < combos_cnt; ++i) {
                for (int j = 0; j < lists_len[L]; ++j) {
                    int p = lists[L][j];
                    // union of combos[i].pr and p (avoid duplicate primes)
                    int *tmp = (int*)malloc(sizeof(int)*(combos[i].len + 1));
                    int t = 0;
                    // copy existing
                    for (int a=0;a<combos[i].len;++a) tmp[t++]=combos[i].pr[a];
                    bool found=false;
                    for (int a=0;a<combos[i].len;++a) if (combos[i].pr[a]==p) { found=true; break; }
                    if (!found) tmp[t++]=p;
                    // sort & unique
                    for (int x=0;x<t-1;++x) for (int y=x+1;y<t;++y) if (tmp[x] > tmp[y]) { int tm=tmp[x]; tmp[x]=tmp[y]; tmp[y]=tm; }
                    int u=1;
                    for (int x=1;x<t;++x) if (tmp[x] != tmp[x-1]) tmp[u++] = tmp[x];
                    // add to newc
                    combo_t nc;
                    nc.len = u;
                    nc.pr = (int*)malloc(sizeof(int)*u);
                    memcpy(nc.pr, tmp, sizeof(int)*u);
                    free(tmp);
                    if (newc_cnt >= newc_cap) {
                        newc_cap = (newc_cap==0)?8:newc_cap*2;
                        newc = (combo_t*)realloc(newc, sizeof(combo_t)*newc_cap);
                    }
                    newc[newc_cnt++] = nc;
                }
            }
            // free old combos
            for (int i=0;i<combos_cnt;++i) free(combos[i].pr);
            free(combos);
            combos = newc;
            combos_cnt = newc_cnt;
            combos_cap = newc_cap;
        }

        // Now combos contains all prime-index combinations covering all remaining minterms.
        // choose combos that give minimal number of primes; break ties by total literal count (sum literals of implicants)
        int best_len = INT_MAX;
        int best_literal_sum = INT_MAX;
        int best_idx = -1;
        for (int ci = 0; ci < combos_cnt; ++ci) {
            int len = combos[ci].len;
            if (len > best_len) continue;
            // compute literal sum
            int lit_sum = 0;
            for (int a = 0; a < combos[ci].len; ++a) lit_sum += literal_count(primes[combos[ci].pr[a]].pattern);
            if (len < best_len || (len == best_len && lit_sum < best_literal_sum)) {
                best_len = len;
                best_literal_sum = lit_sum;
                best_idx = ci;
            }
        }

        // select best combination primes
        if (best_idx != -1) {
            for (int a = 0; a < combos[best_idx].len; ++a) prime_selected[combos[best_idx].pr[a]] = true;
        }

        // cleanup
        for (int i=0;i<remaining;++i) free(lists[i]);
        free(lists);
        free(lists_len);
        for (int i=0;i<combos_cnt;++i) free(combos[i].pr);
        free(combos);
    }

    // Gather selected implicant patterns
    int out_cnt = 0;
    char **out = NULL;
    for (int p = 0; p < primes_cnt; ++p) {
        if (prime_selected[p]) {
            out = (char**)realloc(out, sizeof(char*)*(out_cnt+1));
            out[out_cnt++] = strdup(primes[p].pattern);
        }
    }

    // Return results
    *result_patterns = out;
    *result_count = out_cnt;

    // cleanup
    for (int p=0;p<primes_cnt;++p) free(chart[p]);
    free(chart);
    free(minterm_vals);
    free(prime_selected);
    free(col_covered);
    free_implicants(primes, primes_cnt);
}

/* Convert pattern (e.g. "1-0") to product term string using variable order in var_stack (var stack head is lowest? we reversed earlier in caller)
   var_stack is a stack* with head pointing to first var in left-to-right order for printing */
char* pattern_to_term_sop(const char *pattern, stack* var_stack) {
    // var_stack nodes correspond to pattern indices in order; we assume var_stack->head is first var (left-most)
    int n = strlen(pattern);
    // collect variables in an array in order
    char *vars = (char*)malloc(n+1);
    node *itr = var_stack->head;
    int idx = 0;
    while (itr && idx < n) {
        vars[idx++] = itr->data;
        itr = itr->next;
    }
    vars[idx] = '\0';
    // create term
    char *term = (char*)malloc(n*3 + 2); // rough upper bound
    term[0] = '\0';
    bool any = false;
    for (int i=0;i<n;++i) {
        if (pattern[i] == '-') continue;
        any = true;
        char buf[8];
        if (pattern[i] == '1') snprintf(buf, sizeof(buf), "%c", vars[i]);
        else snprintf(buf, sizeof(buf), "%c'", vars[i]);
        strcat(term, buf);
    }
    if (!any) {
        // pattern covers all variables -> function is 1
        free(term);
        term = strdup("1");
    }
    free(vars);
    return term;
}

/* ---- Integrate with your func() output and JSON ---- */

int* parseIntList(char* str, int* count) {
    int* list = NULL;
    *count = 0;
    char* token = strtok(str, ",");
    while (token != NULL) {
        list = (int*)realloc(list, sizeof(int) * (*count + 1));
        if (list == NULL) {
            // Handle realloc failure
            printf("{\"error\":\"Memory allocation failure\"}");
            exit(1); 
        }
        list[*count] = atoi(token);
        (*count)++;
        token = strtok(NULL, ",");
    }
    return list;
}

/**
 * Creates a stack of variables from a comma-separated string (e.g., "A,B,C").
 * Assumes 'get_stack_size' is a function you have.
 */
stack* getVarStackFromString(char* str) {
    stack* var = newStack();
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] >= 'A' && str[i] <= 'Z' || str[i] >= 'a' && str[i] <= 'z') {
            push(var, str[i]); // Assumes you have a push(stack*, char)
        }
    }
    // Note: The original function reversed the stack.
    // We'll assume the user enters "A,B,C" and we want "A,B,C"
    // in that order, so we reverse it to match the original logic.
    reverseStack(var); // Assumes you have reverseStack()
    return var;
}

// You might need this helper if you don't have one
int get_stack_size(stack* st) {
    int size = 0;
    node* itr = st->head;
    while(itr) {
        size++;
        itr = itr->next;
    }
    return size;
}


/**
 * Converts a binary pattern from Quine-McCluskey into a POS term.
 * Example: pattern "01-" with vars [A, B, C] becomes "(A+B')"
 */
char* pattern_to_term_pos(const char *pattern, stack *var) {
    // Allocate memory for the term string. Size 256 is arbitrary but likely safe.
    char *term = (char*)malloc(sizeof(char) * 256);
    if (term == NULL) return NULL; // Malloc check
    
    memset(term, 0, 256); // Clear memory
    
    node* itr = var->head;
    int len = strlen(pattern);
    bool first_var_in_term = true; // To manage the '+' separator

    sprintf(term, "("); // Start the POS term

    for (int i = 0; i < len; i++) {
        // Only process if the character is '0' or '1'
        if (pattern[i] != '-') {
            // Add a '+' if this is NOT the first variable in this term
            if (!first_var_in_term) {
                sprintf(term + strlen(term), "+");
            }

            if (pattern[i] == '0') {
                // POS rule: 0 means the variable is NOT inverted
                sprintf(term + strlen(term), "%c", itr->data);
            } else if (pattern[i] == '1') {
                // POS rule: 1 means the variable IS inverted
                sprintf(term + strlen(term), "%c'", itr->data);
            }
            first_var_in_term = false;
        }
        
        // Always advance the variable iterator
        if (itr) {
            itr = itr->next;
        }
    }
    
    sprintf(term + strlen(term), ")"); // End the POS term
    
    // strdup is not standard C, but if you have it: return strdup(term);
    // Otherwise, since we malloc'd, we can just return the pointer.
    // The caller is responsible for free(term).
    return term;
}

void handle_expression(char* expr, Mode mode) {
    // --- 1. SETUP (Identical for both) ---
    stack* st = newStack();
    if(isValidExpression(expr)) {
        inf_to_posf(expr,st);
    } else {
        printf("{\"error\":\"Invalid Expression\"}");
        free_stack(st);
        return;
    }

    bnode** minterms = getMinterms(st);
    stack* var = getDistinctVar(st);
    reverseStack(var);
    node* itr = var->head;
    int length = 0;
    while(itr) { length++; itr = itr->next; }
    int rows = pow(2, length);
    int cols = length + 1;

    // Print JSON: postfix, headers, rows (Identical)
    printf("{\"postfix\":\"");
    node* itr2 = st->head;
    while(itr2) { printf("%c", itr2->data); itr2 = itr2->next; }
    printf("\",");

    printf("\"headers\":[");
    itr = var->head;
    while(itr) {
        printf("\"%c\"", itr->data);
        if (itr->next != NULL) printf(",");
        itr = itr->next;
    }
    printf(",\"Output\"],");

    printf("\"rows\":[");
    for(int i = 0; i < rows; i++) {
        printf("[");
        for(int j = 0; j < cols; j++) {
            printf("%d", minterms[i][j].val);
            if (j < cols - 1) printf(",");
        }
        printf("]");
        if (i < rows - 1) printf(",");
    }
    printf("],");

    // --- 2. MODE-SPECIFIC LOGIC ---
    
    // Set variables based on mode
    int target_val = (mode == SOP) ? 1 : 0;
    char* output_if_zero = (mode == SOP) ? "0" : "1";
    char* output_if_all = (mode == SOP) ? "1" : "0";
    char* output_if_is_one = (mode == SOP) ? "1" : "0";
    PatternToTermFunc pattern_func = (mode == SOP) ? pattern_to_term_sop : pattern_to_term_pos;
    char* joiner = (mode == SOP) ? "+" : ""; // POS terms are just `(A+B)(C+D)`

    // Collect terms (minterms or maxterms)
    int *term_list = NULL;
    int term_count = 0;
    for (int i=0;i<rows;++i) {
        if (minterms[i][length].val == target_val) {
            term_list = (int*)realloc(term_list, sizeof(int)*(term_count+1));
            term_list[term_count++] = i;
        }
    }

    // Edge cases
    if (term_count == 0) {
        printf("\"minimized\":\"%s\"}", output_if_zero);
        // ... cleanup ... (identical, see below)
        goto cleanup; // Use goto for a single cleanup point
    }
    if (term_count == rows) {
        printf("\"minimized\":\"%s\"}", output_if_all);
        // ... cleanup ...
        goto cleanup;
    }

    // Run Quine-McCluskey (Identical)
    char **res_patterns = NULL;
    int res_cnt = 0;
    quine_mccluskey(term_list, term_count, length, &res_patterns, &res_cnt);

    // Convert patterns to human-readable terms
    if (res_cnt == 0) {
        printf("\"minimized\":\"\"}");
    } else {
        bool is_one = false;
        for (int i=0;i<res_cnt;++i) if (strcmp(res_patterns[i], "1")==0) is_one = true;
        
        if (is_one) {
            printf("\"minimized\":\"%s\"}", output_if_is_one);
        } else {
            printf("\"minimized\":\"");
            for (int i=0;i<res_cnt;++i) {
                // Call the correct function (SOP or POS) using the function pointer
                char *term = pattern_func(res_patterns[i], var);
                
                printf("%s", term);
                
                // Use the correct joiner (SOP or POS)
                if (i < res_cnt - 1) printf("%s", joiner);
                
                free(term);
            }
            printf("\"}");
        }
    }

    // --- 3. CLEANUP (Identical) ---
cleanup:
    // This 'goto' label provides a single exit point for cleanup
    // This is a safe and common use of goto in C for resource management
    if (res_patterns) {
        for (int i=0;i<res_cnt;++i) free(res_patterns[i]);
        free(res_patterns);
    }
    for(int i=0;i<rows;i++) free(minterms[i]);
    free(minterms);
    free_stack(var);
    free_stack(st);
    free(term_list); // Renamed from minlist/maxlist
}

void handle_expression_sop(char* expr) {
    handle_expression(expr, SOP);
}

void handle_expression_pos(char* expr) {
    handle_expression(expr, POS);
}


void handle_minterms(char* var_str, char* minterms_str)
{
    // --- START OF MODIFICATIONS ---
    
    // 1. Parse and clean the variable string
    char clean_vars[30]; // Buffer for 26 letters + padding
    clean_vars[0] = '\0';
    
    // This is the robust function from our previous conversation
    int length = parse_variable_list(var_str, clean_vars);
    
    // 2. Create the stack from the *clean* string
    stack* var = getVarStackFromCleanString(clean_vars);
    
    // --- END OF MODIFICATIONS ---
    int mincount = 0;
    int* minlist = parseIntList(minterms_str, &mincount);
    if (length == 0 || mincount == 0) {
         printf("{\"error\":\"Invalid minterm or variable input\"}");
         if(var) free_stack(var);
         if(minlist) free(minlist);
         return;
    }
    int max_term_len = (4 * length) + 1;
    int num_joiners = (mincount > 0) ? mincount - 1 : 0;
    // Total size = (all terms) + (all joiners) + (1 for null)
    int s = (max_term_len * mincount) + num_joiners + 1;
    char* expr = (char*)malloc(s * sizeof(char));
    if (expr == NULL) { /* Handle malloc failure */ return; }
    expr[0] = '\0'; // Start with an empty string
    for(int i = 0; i < mincount; i++)
    {
        node* itr = var->head;
        int* bi = decimal_to_binary(minlist[i], length);
        // Add opening parenthesis for the term
        sprintf(expr + strlen(expr), "(");
        for(int j = 0; j < length; j++)
        {
            // Add the variable
            sprintf(expr + strlen(expr), "%c", itr->data);
            // Add the apostrophe if '0'
            if(bi[j] == 0)
            {
                sprintf(expr + strlen(expr), "'");
            }
            // Add AND operator ('.') if not the last var in the term
            if (j < length - 1) {
                sprintf(expr + strlen(expr), ".");
            } 
            itr = itr->next;
        } 
        // Add closing parenthesis
        sprintf(expr + strlen(expr), ")");
        // Add OR operator ('+') if not the last term
        if (i < mincount - 1) {
            sprintf(expr + strlen(expr), "+");
        }
        free(bi); 
    }
    handle_expression_sop(expr);
    //Cleanup
    free(expr);
    free_stack(var);
    free(minlist);
}

void handle_maxterms(char* var_str, char* maxterms_str)
{
    // --- START OF MODIFICATIONS ---
    
    // 1. Parse and clean the variable string
    char clean_vars[30]; // Buffer for 26 letters + padding
    clean_vars[0] = '\0';
    
    // This is the robust function from our previous conversation
    int length = parse_variable_list(var_str, clean_vars);
    
    // 2. Create the stack from the *clean* string
    stack* var = getVarStackFromCleanString(clean_vars);
    
    // --- END OF MODIFICATIONS ---
    int maxcount = 0;
    int* maxlist = parseIntList(maxterms_str, &maxcount);
    // 2. Input Validation
    if (length == 0 || maxcount == 0) {
         printf("{\"error\":\"Invalid maxterm or variable input\"}");
         if(var) free_stack(var);
         if(maxlist) free(maxlist);
         return;
    }
    // 3. Safe Memory Allocation
    // Calculate the max possible string size.
    // A single term is (A'+B'+C') -> (3*length + 1) chars
    // We have 'maxcount' terms.
    // We have (maxcount - 1) joiners ('.')
    // And 1 null terminator.
    int max_term_len = (3 * length) + 1; // e.g., (A'+B') is 7 chars. 3*2+1=7.
    int num_joiners = (maxcount > 0) ? maxcount - 1 : 0;
    int s = (maxcount * max_term_len) + num_joiners + 1;
    
    char* expr = (char*)malloc(s * sizeof(char));
    if (expr == NULL) { 
        printf("{\"error\":\"Memory allocation failed\"}");
        if(var) free_stack(var);
        if(maxlist) free(maxlist);
        return;
    }
    // Start with an empty string
    expr[0] = '\0'; 
    // 4. Build the Full POS String
    for(int i = 0; i < maxcount; i++)
    {
        node* itr = var->head;
        // e.g., maxterm 2 (10) for 3 vars is 010
        int* bi = decimal_to_binary(maxlist[i], length); 
        // Start the sum term
        sprintf(expr + strlen(expr), "(");
        
        for(int j = 0; j < length; j++)
        {
            // Add '+' joiner if not the first variable
            if (j > 0) {
                sprintf(expr + strlen(expr), "+");
            }
            // POS Rule: 0 = Normal, 1 = Inverted
            if(bi[j] == 0) {
                sprintf(expr + strlen(expr), "%c", itr->data);
            } else {
                sprintf(expr + strlen(expr), "%c'", itr->data);
            }
            itr = itr->next;
        }
        // Close the sum term
        sprintf(expr + strlen(expr), ")");
        // Add '.' joiner if not the last term
        if (i < maxcount - 1) {
            sprintf(expr + strlen(expr), ".");
        }
        free(bi); // Free the binary array
    }
    // 5. Pass to Expression Handler
    // 'expr' is now a string like "(A+B+C).(A+B+C')"
    handle_expression_pos(expr);
    // 6. Cleanup
    free(expr);
    free_stack(var);
    free(maxlist);
}

int main() {
    char input_type_str[5];
    char line1_str[1000]; // For variables or expression
    char line2_str[1000]; // For min/max terms

    // 1. Read the input type (e.g., "1", "2", or "3")
    if (!fgets(input_type_str, sizeof(input_type_str), stdin)) return 0;
    int type = atoi(input_type_str);

	char* processed = NULL;
    switch(type) {
        case 1: // Expression sop
            if (!fgets(line1_str, sizeof(line1_str), stdin)) return 0;
            line1_str[strcspn(line1_str, "\n")] = '\0';
            string_to_uppercase(line1_str);
            processed = addImplicitANDs(line1_str);
            handle_expression_sop(processed); // Your original function
            free(processed);
            break;
            
        case 2: // Expression pos
            if (!fgets(line1_str, sizeof(line1_str), stdin)) return 0;
            line1_str[strcspn(line1_str, "\n")] = '\0';
            string_to_uppercase(line1_str);
            processed = addImplicitANDs(line1_str);
            handle_expression_pos(processed); // Your original function
            free(processed);
            break;

        case 3: // Minterms
            if (!fgets(line1_str, sizeof(line1_str), stdin)) return 0; // Var list
            if (!fgets(line2_str, sizeof(line2_str), stdin)) return 0; // Term list
            line1_str[strcspn(line1_str, "\n")] = '\0';
            line2_str[strcspn(line2_str, "\n")] = '\0';
			string_to_uppercase(line1_str);
            handle_minterms(line1_str, line2_str);
            break;

        case 4: // Maxterms
            if (!fgets(line1_str, sizeof(line1_str), stdin)) return 0; // Var list
            if (!fgets(line2_str, sizeof(line2_str), stdin)) return 0; // Term list
            line1_str[strcspn(line1_str, "\n")] = '\0';
            line2_str[strcspn(line2_str, "\n")] = '\0';
            string_to_uppercase(line1_str);
            handle_maxterms(line1_str, line2_str);
            break;

        default:
            printf("{\"error\":\"Invalid input type\"}");
            break;
    }

    return 0;
}
