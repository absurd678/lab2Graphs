#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>

#define MAX_VERTICES 1000

typedef struct Node
{
    int vertex;        // Концевая вершина ребра
    int capacity;      // Ёмкость ребра
    int flow;          // Текущий поток через ребро
    struct Node *next; // Указатель на следующий элемент в списке
} Node;

typedef struct
{
    Node *head; // Указатель на голову списка смежности
} AdjacencyList;

typedef struct
{
    int numVertices;         // Количество вершин в графе
    AdjacencyList *adjLists; // Список смежности
} Graph;

void printGraph(Graph *graph)
{
    if (graph == NULL)
    {
        printf("Graph is NULL\n");
        return;
    }

    printf("Graph with %d vertices:\n", graph->numVertices);
    for (int i = 0; i < graph->numVertices; i++)
    {
        printf("Adjacency list of vertex %d:\n", i);
        Node *current = graph->adjLists[i].head;
        while (current)
        {
            printf(" -> (%d, capacity: %d, flow: %d)", current->vertex, current->capacity, current->flow);
            current = current->next;
        }
        printf("\n");
    }
}

// Создание нового узла
Node *createNode(int vertex, int capacity)
{
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->vertex = vertex;
    newNode->capacity = capacity;
    newNode->flow = 0; // Начальный поток равен нулю
    newNode->next = NULL;
    return newNode;
}

// Создание графа с заданным количеством вершин
Graph *createGraph(int vertices)
{
    Graph *graph = (Graph *)malloc(sizeof(Graph));
    graph->numVertices = vertices;
    graph->adjLists = (AdjacencyList *)malloc(vertices * sizeof(AdjacencyList));
    for (int i = 0; i < vertices; i++)
    {
        graph->adjLists[i].head = NULL; // Инициализация списка смежности
    }
    return graph;
}

// Добавление ребра в граф
void addEdge(Graph *graph, int src, int dest, int capacity)
{
    Node *newNode = createNode(dest, capacity);
    newNode->next = graph->adjLists[src].head;
    graph->adjLists[src].head = newNode;

    // Добавляем обратное ребро с нулевой ёмкостью
    newNode = createNode(src, 0); // Для обратного ребра
    newNode->next = graph->adjLists[dest].head;
    graph->adjLists[dest].head = newNode;
}

// Функция поиска в ширину для нахождения путей
int bfs(Graph *graph, int source, int sink, int *parent)
{
    int visited[MAX_VERTICES] = {0}; // Массив для отслеживания посещенных вершин
    visited[source] = 1;             // Отметим начальную вершину как посещенную
    int queue[MAX_VERTICES], front = 0, rear = 0;
    queue[rear++] = source; // Добавим начальную вершину в очередь

    while (front < rear)
    {
        int u = queue[front++]; // Извлечение вершины из очереди

        for (Node *current = graph->adjLists[u].head; current; current = current->next)
        {
            int v = current->vertex;

            // Проверяем, был ли узел посещен и есть ли доступный поток
            if (!visited[v] && current->capacity - current->flow > 0)
            {
                parent[v] = u; // Устанавливаем родителя
                if (v == sink) // Если достигли целевой вершины
                    return 1;  // Возвращаем успех

                visited[v] = 1;    // Отмечаем как посещенный
                queue[rear++] = v; // Добавляем в очередь
            }
        }
    }
    return 0; // Не нашли путь
}

// Основная функция алгоритма Форда-Фалкерсона
int fordFulkerson(Graph *graph, int source, int sink)
{
    int maxFlow = 0;          // Начальный максимальный поток
    int parent[MAX_VERTICES]; // Массив для хранения пути

    // Увеличиваем поток, пока существует путь из source в sink
    while (bfs(graph, source, sink, parent))
    {
        int pathFlow = INT_MAX;

        // Находим минимальную ёмкость по пути
        for (int v = sink; v != source; v = parent[v])
        {
            int u = parent[v];
            for (Node *current = graph->adjLists[u].head; current; current = current->next)
            {
                if (current->vertex == v)
                {
                    pathFlow = (pathFlow < current->capacity - current->flow) ? pathFlow : current->capacity - current->flow;
                    break; // Выход из цикла
                }
            }
        }

        // Обновляем потоки по найденному пути
        for (int v = sink; v != source; v = parent[v])
        {
            int u = parent[v];
            for (Node *current = graph->adjLists[u].head; current; current = current->next)
            {
                if (current->vertex == v)
                {
                    current->flow += pathFlow; // Увеличиваем поток по ребру
                    break;                     // Выход из цикла
                }
            }
            for (Node *current = graph->adjLists[v].head; current; current = current->next)
            {
                if (current->vertex == u)
                {
                    current->flow -= pathFlow; // Уменьшаем поток по обратному ребру
                    break;                     // Выход из цикла
                }
            }
        }

        maxFlow += pathFlow; // Увеличиваем общий максимальный поток
    }
    return maxFlow; // Возвращаем максимальный поток
}

// Чтение графа из бинарного файла

Graph *readGraphFromFile(const char *filename, int *source, int *sink) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Не удалось открыть файл");
        exit(EXIT_FAILURE);
    }

    int isBinary = 0;
    if (strstr(filename, ".bl") != NULL) {
        isBinary = 1;
        file = fopen(filename, "rb");
    }

    int16_t numVertices;
    if (isBinary) {
        fread(&numVertices, sizeof(int16_t), 1, file); // Чтение количества вершин
    } else {
        fscanf(file, "%hd", &numVertices); // Чтение количества вершин из текстового файла
    }

    printf("numVertices = %d\n", numVertices);

    Graph *graph = createGraph(numVertices);
    int *inDegree = (int *)calloc(numVertices, sizeof(int));
    int *outDegree = (int *)calloc(numVertices, sizeof(int));

    int16_t src, dest, capacity;
    
    while ((isBinary && fread(&src, sizeof(int16_t), 1, file) &&
            fread(&dest, sizeof(int16_t), 1, file) &&
            fread(&capacity, sizeof(int16_t), 1, file)) ||
           (!isBinary && fscanf(file, "%hd %hd %hd", &src, &dest, &capacity) == 3)) {
        addEdge(graph, src, dest, capacity); // Добавляем рёбра
        outDegree[src]++;
        inDegree[dest]++;
    }

    fclose(file);

    *source = -1; // Начальная вершина
    *sink = -1;   // Конечная вершина

    // Определяем начальную и конечную вершины
    for (int i = 0; i < numVertices; i++) {
        if (*source == -1 && outDegree[i] > 0 && inDegree[i] == 0) {
            *source = i; // Вершина с исходящими рёбрами, но без входящих
        }
        if (*sink == -1 && inDegree[i] > 0 && outDegree[i] == 0) {
            *sink = i; // Вершина с входящими рёбрами, но без исходящих
        }
    }

    free(inDegree);
    free(outDegree);

    if (*source == -1 || *sink == -1) {
        fprintf(stderr, "Не удалось определить начальную или конечную вершину\n");
        exit(EXIT_FAILURE);
    }

    return graph; // Возвращаем созданный граф
}

// Запись результата в файл
void writeResultToFile(const char *filename, Graph *graph, int maxFlow, int source, int sink)
{
    FILE *file = fopen(filename, "w");
    if (!file)
    {
        perror("Не удалось открыть файл для записи");
        exit(EXIT_FAILURE);
    }

    fprintf(file, "Value of maximum flow from %d to %d vertices: %d\n", source, sink, maxFlow);
    fprintf(file, "Flow:\n");

    // Записываем все рёбра с положительным потоком
    for (int u = 0; u < graph->numVertices; u++)
    {
        for (Node *current = graph->adjLists[u].head; current; current = current->next)
        {
            if (current->flow > 0)
            {
                fprintf(file, "(%d, %d, %d)\n", u, current->vertex, current->flow);
            }
        }
    }

    fclose(file); // Закрываем файл
}
//
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Использование: %s <имя входного файла> [-o <имя выходного файла>]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *inputFilename = argv[1];
    // const char *inputFilename = "list_of_edges_t2_001.txt";
    const char *outputFilename = "output.txt";

    // Проверка на наличие параметра -o для имени выходного файла
    for (int i = 2; i < argc; i++)
    {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc)
        {
            outputFilename = argv[i + 1];
            break;
        }
    }

    int source, sink;
    Graph *graph = readGraphFromFile(inputFilename, &source, &sink);
    //Graph *graph = readGraphFromStdin(&source, &sink);
    int maxFlow = fordFulkerson(graph, source, sink);
    writeResultToFile(outputFilename, graph, maxFlow, source, sink);

    // Освобождаем память
    for (int i = 0; i < graph->numVertices; i++)
    {
        Node *current = graph->adjLists[i].head;
        while (current)
        {
            Node *temp = current;
            current = current->next;
            free(temp);
        }
    }
    free(graph->adjLists);
    free(graph);

    return EXIT_SUCCESS;
}
