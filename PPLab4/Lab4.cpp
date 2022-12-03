#include <iostream>
#include <chrono>
#include <omp.h>

using namespace std;
using namespace std::chrono;

/*
	Паралельне програмування: Лабораторна робота №4 (ЛР4)

	Варіант: 11
	Функція: e = ((p * (A * MB)) * (B * (MZ * MR)) + min(B)

	Автор: Бондаренко Роман Ігорович, група ІО-03
	Дата: 04/12/2022
*/

const int N = 4;
const int P = 4;
const int H = N / P;

int e, p;
int A[N], B[N];
int MB[N][N], MZ[N][N], MR[N][N];

int C[N], D[N], E[N];
int MA[N][N];
int a = 0;
int m = INT32_MAX;

void fill_matrix(int matrix[N][N]);
void fill_vector(int vector[N]);

void vector_matrix_mult(int from, int to, int res[N], int vector[N], int matrix[N][N]);
int vector_min(int from, int to, int vector[N]);

int main() {

	omp_lock_t Lock1;
	omp_init_lock(&Lock1);

	omp_lock_t Lock2;
	omp_init_lock(&Lock2);

	cout << endl << "N = " << N << endl;
	cout << endl << "Starting threads..." << endl;

	auto start = high_resolution_clock::now();

	#pragma omp parallel num_threads(P)
	{
		int tid = omp_get_thread_num() + 1;
		cout << endl << "Thread T" << tid << " started" << endl;

		int tstart = (tid - 1) * H;
		int tend = tstart + H;

		// 1. Введення даних.
		switch (tid) {
			case 1: {
				fill_matrix(MZ);
				break;
			}
			case 2: {
				fill_vector(A);
				fill_matrix(MR);
				break;
			}
			case 3: {
				fill_matrix(MB);
				fill_vector(B);
				p = 1;
				break;
			}
		}

		// 2. Сигнал задачам Т2, Т3, Т4 про завершення введення даних.
		// 3. Чекати сигнал від інших задач про завершення введення даних.
		#pragma omp barrier

		// 4. Обчислення 1: CH = A * MBH.
		vector_matrix_mult(tstart, tend, C, A, MB);

		// 5. Копіювання: pi = p.
		int pi;
		#pragma omp critical
			pi = p;

		// 6. Обчислення 2: DH = pi * CH.
		for (int i = tstart; i < tend; i++) {
			D[i] = pi * C[i];
		}

		// 7. Обчислення 3: MAH = MZ * MRH.
		for (int i = 0; i < N; i++) {
			for (int j = tstart; j < tend; j++) {
				int res = 0;
				for (int k = 0; k < N; k++) {
					res += MZ[i][k] * MR[k][j];
				}
				MA[i][j] = res;
			}
		}

		// 8. Обчислення 4: EH = B * MAH.
		vector_matrix_mult(tstart, tend, E, B, MA);

		// 9. Обчислення 5: ai = DH * EH.
		int ai = 0;
		for (int i = tstart; i < tend; i++) {
			ai += D[i] * E[i];
		}

		// 10. Обчислення 6: a = a + ai.
		omp_set_lock(&Lock1);
		a += ai;
		omp_unset_lock(&Lock1);

		// 11. Обчислення 7: mi = min(BH).
		int mi = vector_min(tstart, tend, B);

		// 12. Обчислення 8: m = min(m, m1).
		omp_set_lock(&Lock2);
		m = mi < m ? mi : m;
		omp_unset_lock(&Lock2);

		// 13 (1, 3, 4). Сигнал задачі Т2 про завершення обчислення a.
		// 13 (2). Чекати сигнал від задач Т1, Т3, Т4 про завершення обчислення a.
		// 14 (1, 3, 4). Сигнал задачі Т2 про завершення обчислення m.
		// 14 (2). Чекати сигнал від задач Т1, Т3, Т4 про завершення обчислення m.
		#pragma omp barrier

		if (tid == 2) {
			// 15 (2). Обчислення 9: e = a + m.
			e = a + m;

			// 16 (2). Виведення результату e.
			cout << endl << "Thread T" << tid << " - Answer e: " << e << endl;
		}

		cout << endl << "Thread T" << tid << " finished" << endl;
	}

	auto end = high_resolution_clock::now();

	cout << endl << "All thread finished, execution time is " << duration_cast<milliseconds>(end - start).count() << " ms" << endl;

	return 0;
}

void fill_matrix(int matrix[N][N]) {
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++) {
			matrix[i][j] = 1;
		}
	}
}

void fill_vector(int vector[N]) {
	for (int i = 0; i < N; i++) {
		vector[i] = 1;
	}
}

void vector_matrix_mult(int from, int to, int res[N], int vector[N], int matrix[N][N]) {
	for (int i = from; i < to; i++) {
		for (int j = 0; j < N; j++) {
			res[i] += vector[j] * matrix[j][i];
		}
	}
}

int vector_min(int from, int to, int vector[N]) {
	int min = vector[from];
	for (int i = from; i < to; i++) {
		if (vector[i] < min) {
			min = vector[i];
		}
	}
	return min;
}