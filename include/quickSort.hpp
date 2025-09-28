#ifndef quickSortHPP
#define quickSortHPP

// Stats:   Best,   Average,    Worst,  Memory,     Stable
//      n log(n),   n log(n),   n²,     log n,      No

#include <algorithm>

using namespace std;

namespace quickSort {
    template <typename T>
    int partition(T& list, int low, int high) {
        int pivot = list[low];
        int i = low - 1;
        int j = high + 1;

        while (true) {
            do {
                i++;
            } while (list[i] < pivot);

            do {
                j--;
            } while (list[j] > pivot);

            if (i >= j) {
                return j;
            }

            swap(list[i], list[j]);
        }
    }

    template <typename T>
    int partitionInverted(T& list, int low, int high) {
        int pivot = list[low];
        int i = low - 1;
        int j = high + 1;

        while (true) {
            do {
                i++;
            } while (list[i] > pivot);

            do {
                j--;
            } while (list[j] < pivot);

            if (i >= j) {
                return j;
            }

            swap(list[i], list[j]);
        }
    }

    template <typename T>
    void quickSort(T& list, int low, int high) {
        if (low >= 0 && high >= 0 && low < high) {
            int p = partition(list, low, high);
            quickSort(list, low, p);
            quickSort(list, p + 1, high);
        }
    }

    template <typename T>
    void quickSortInverted(T& list, int low, int high) {
        if (low >= 0 && high >= 0 && low < high) {
            int p = partitionInverted(list, low, high);
            quickSortInverted(list, low, p);
            quickSortInverted(list, p + 1, high);
        }
    }

    /*
    * Wrapper function for QuickSort
    * Structure/Container needs Indexing Abilities!
    */
    template <typename T>
    void quickSort(T& list, bool inverted = false) {
        int length = list.size();
        if (inverted) {
            quickSortInverted(list, 0, length - 1);
        } else {
            quickSort(list, 0, length - 1);
        }
    }
}

#endif