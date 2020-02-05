#include "compressor.h"

#define DEBUG if(0)

void search_huff_tree (node* tree, unsigned char item, int path[], int i, int* found) {
	if (tree == NULL) {
		if (*found != 1) path[i + 1] = -1;
		return;
	} else if (is_leaf(tree) && tree->item == item) {
		*found = 1;
		return;
	}
	if(*found != 1) {
	  path[i] = 0;
	} 	
	search_huff_tree(tree->left, item, path, i - 1, found);
	if(*found != 1) {
	  path[i] = 1;
	}
	search_huff_tree(tree->right, item, path, i - 1, found);
	if (*found != 1) path[i + 1] = -1;
}

void dec_to_bin(int beggin, int rest, int array[], int i) {
	if(beggin >= 1) { 
		rest = beggin % 2;
		beggin = beggin / 2;
		dec_to_bin(beggin, 0, array, i+1);
		printf ("%d", rest);
		array[i] = rest;
	}
}

void get_number_of_nodes(node* tree, int* size) { // vai contar o numero de nós da arvore.
	if (tree == NULL) {
		return;
	}
	if (is_leaf(tree) && (tree -> item == '*' || tree -> item == '\\')) {
		(*size)++;
	}
	(*size)++;
	get_number_of_nodes(tree->left, size);
	get_number_of_nodes(tree->right, size);
}

// Calcula quantos bits o caracter ocupa no novo mapeamento
int changed_positions(int array[], int size) {
	int total = 0;
	  for(int i = 0; i < size; i++) {
		if (array[i] != -1) {
			total++;
		}
	}
	return total;
}

void print_bits(FILE *file, FILE *compressed_file, hash_table *mapping, int trash_size) {
	unsigned char c; 
	unsigned char byte = 0; 
	int i = 0; 
	while (fscanf (file, "%c", &c) != EOF) { 
		int h = (int) c; 
		int n = changed_positions(mapping->table[h]->new_mapping, 9);
		for(int j = 8; j >= 9 - n; j--) {
			if (i == 8) { // Completei um byte! :)
				fprintf(compressed_file, "%c", byte);
				byte = 0;
				i = 0;
			}
			if (mapping->table[h]->new_mapping[j] == 1) {
				// printf("Setando o bit %d\n", 7 - i);
				byte = set_bit(byte, 0);
			}
			if (i < 7) byte <<= 1;
			/* printf("Byte: ");
			for(int i = 7; i >= 0; i--) {
				printf("%d", is_bit_i_set(byte, i) ? 1 : 0);
			}
			printf ("\n"); */
			i++; 
		}
	}
	// printf("\n");
	fprintf(compressed_file, "%c", byte);
	// byte <<= 1;
	byte <<= trash_size;
	// if (trash_size != 0) {
	// 	fprintf(compressed_file, "%c", byte);
	// }
}

void get_new_mapping(hash_table *mapping, node* huff_tree) {
	for (int i = 0; i < HASH_SIZE; i++) {
		if (mapping->table[i] != NULL) {
			int path[9];
			memset(path, -1, sizeof(path));
			unsigned char item = mapping->table[i]->key;
			int found = 0;

			printf("aq\n");
			printf("i = %d\n", i);

			search_huff_tree(huff_tree, item, path, 8, &found);	

			for(int e = 0; e < 9; e++) {
				mapping->table[i]->new_mapping[e] = path[e];
			}
		}
	}
}

int get_trash_size(hash_table *mapping) {
	int i;
	int bits_usados = 0;
	for (i = 0; i < HASH_SIZE; i++) {
		if (mapping->table[i] != NULL) { // Tenho que multiplicar quantos bits um unico caracter daquele ocupa pela quantidade de vezes que ele aparece no texto
			int n = changed_positions(mapping->table[i]->new_mapping, 9);
			int m = mapping->table[i]->frequency;
			bits_usados += n * m; 
		}
	}
	return (8 - (bits_usados % 8) == 8 ? 0 : 8 - (bits_usados % 8));
}

void get_frequency(FILE *file, hash_table *mapping) {
	unsigned char unit;
	while (fscanf(file,"%c",&unit) != EOF) { // vai até o EOF.
		int h = (int) unit;
		if (!contains_key(mapping, unit)) { // Se eu ainda não adicionei aquele caracter;
			put(mapping, unit, 1);
		} else {
			mapping->table[h]->frequency++;
		}
	  }
}

void print_new_mapping(hash_table *mapping) {
	for (int  i = 0; i < HASH_SIZE; i++) {
		if (mapping->table[i] != NULL) {
			printf("%c = ", mapping->table[i]->key);
			for (int j = 8; j >= 0; j--) {
				printf("%d ", mapping->table[i]->new_mapping[j]);
			}
			printf("\n");
		}
	}
}

queue *create_queue_from_hash(hash_table *mapping, queue *queue) {
	for (int i = 0; i < 256; i++) {
		if (mapping->table[i] != NULL) {
			queue = add(queue, mapping->table[i]->key, mapping->table[i]->frequency);
		}
	}
	return queue;
}

void compress() {
	FILE *arq;
	char file_path[5000];

	while (1) {
		printf("Digite o caminho para o arquivo que deseja comprimir: ");
		scanf ("%[^\n]s", file_path);
		printf("%s\n", file_path);

		arq = fopen(file_path, "rb");
		if (arq == NULL) {
			printf("Não foi possivel encontrar o arquivo!\n");
		} else {
			break;
		}
	}
	
	hash_table* mapping = create_hash_table();
	get_frequency(arq, mapping);

	DEBUG printf("HASH TABLE::::: \n");
	DEBUG print_hash_table(mapping);
	DEBUG printf("\n");

	queue *myqueue = create_queue();
	myqueue = create_queue_from_hash(mapping, myqueue);

	node *huff_tree = build_tree(myqueue);
	DEBUG print_tree(huff_tree, 0);
	
	rewind(arq);
	get_new_mapping(mapping, huff_tree);
	DEBUG print_new_mapping(mapping);

	int tree_size = 0;
	get_number_of_nodes(huff_tree, &tree_size);

	int trash_size = get_trash_size(mapping);

	int array[16];
	memset(array, 0, sizeof(array)); 

	printf ("Tamanho da árvore: %d (", tree_size);
	dec_to_bin(tree_size, 0, array, 0);
	printf (")\n");
	
	printf("Tamanho do lixo: %d (", trash_size);
	dec_to_bin(trash_size, 0, array, 13);
	printf (")\n");
	 
	unsigned short int c = 0;
	c = trash_size;
	c <<= 13;
	c += tree_size;

	strcat(file_path, ".huff");
	FILE *compressed_file = fopen(file_path, "wb"); // cria um arquivo e permite a escrita nele.
	if (compressed_file == NULL) {
		printf ("Não foi possível comprimir o arquivo\n");
		return;
	}
    fprintf(compressed_file, "%c%c", c >> 8, c);

	// printf(">>>>>>>\n");
	print_tree_on_file(compressed_file, huff_tree); // função que imprimi a arvore no arquivo.
	
	print_bits(arq, compressed_file, mapping, trash_size);
	fclose(compressed_file);
	fclose(arq);
	/*compressed_file = fopen(file_path, "r");
	unsigned char ch;
	int count = 0;
	while(fscanf(compressed_file, "%c", &ch) != EOF) {
		if(count < tree_size + 2) {
			count++;
			continue;
		}
		printar_byte(ch);
		printf("|");
	} */
	return;
}