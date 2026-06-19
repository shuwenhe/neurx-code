/**
 * @file quick_sort.cc
 * @brief 快速排序算法的C++实现
 * @author NeurX Code Agent
 * @date 2026-06-18
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <cassert>

/**
 * @brief 分区函数 - 快速排序的核心
 * 
 * 选择最后一个元素作为pivot，将数组分为两部分：
 * - 左边：小于等于pivot的元素
 * - 右边：大于pivot的元素
 * 
 * @param arr 待排序数组
 * @param low 起始索引
 * @param high 结束索引
 * @return int pivot最终位置的索引
 */
int partition(std::vector<int>& arr, int low, int high) {
    int pivot = arr[high];  // 选择最后一个元素作为pivot
    int i = low - 1;  // 小于pivot的区域的最后一个元素索引
    
    // 遍历数组，将小于pivot的元素移到左边
    for (int j = low; j < high; j++) {
        if (arr[j] <= pivot) {
            i++;
            std::swap(arr[i], arr[j]);
        }
    }
    
    // 将pivot放到正确的位置
    std::swap(arr[i + 1], arr[high]);
    return i + 1;
}

/**
 * @brief 快速排序主函数
 * 
 * 使用分治策略：
 * 1. 选择pivot并分区
 * 2. 递归排序左子数组
 * 3. 递归排序右子数组
 * 
 * 时间复杂度：
 * - 平均: O(n log n)
 * - 最坏: O(n²) (当数组已排序或逆序时)
 * - 最好: O(n log n)
 * 
 * 空间复杂度: O(log n) (递归调用栈)
 * 
 * @param arr 待排序数组
 * @param low 起始索引
 * @param high 结束索引
 */
void quickSort(std::vector<int>& arr, int low, int high) {
    if (low < high) {
        // 分区，获取pivot的最终位置
        int pivotIndex = partition(arr, low, high);
        
        // 递归排序pivot左边的子数组
        quickSort(arr, low, pivotIndex - 1);
        
        // 递归排序pivot右边的子数组
        quickSort(arr, pivotIndex + 1, high);
    }
}

/**
 * @brief 快速排序的公共接口
 * 
 * @param arr 待排序数组
 */
void quickSort(std::vector<int>& arr) {
    if (!arr.empty()) {
        quickSort(arr, 0, arr.size() - 1);
    }
}

/**
 * @brief 优化版快速排序 - 三路快速排序
 * 
 * 处理有大量重复元素的情况，将数组分为三部分：
 * - 小于pivot
 * - 等于pivot
 * - 大于pivot
 * 
 * @param arr 待排序数组
 * @param low 起始索引
 * @param high 结束索引
 */
void quickSort3Way(std::vector<int>& arr, int low, int high) {
    if (low >= high) return;
    
    int pivot = arr[low];
    int lt = low;      // arr[low+1..lt] < pivot
    int gt = high;     // arr[gt..high] > pivot
    int i = low + 1;   // arr[lt+1..i-1] == pivot
    
    while (i <= gt) {
        if (arr[i] < pivot) {
            std::swap(arr[lt++], arr[i++]);
        } else if (arr[i] > pivot) {
            std::swap(arr[i], arr[gt--]);
        } else {
            i++;
        }
    }
    
    // 递归排序 < pivot 和 > pivot 的部分
    quickSort3Way(arr, low, lt - 1);
    quickSort3Way(arr, gt + 1, high);
}

/**
 * @brief 三路快速排序的公共接口
 * 
 * @param arr 待排序数组
 */
void quickSort3Way(std::vector<int>& arr) {
    if (!arr.empty()) {
        quickSort3Way(arr, 0, arr.size() - 1);
    }
}

// ========== 测试函数 ==========

/**
 * @brief 打印数组
 */
void printArray(const std::vector<int>& arr, const std::string& label = "") {
    if (!label.empty()) {
        std::cout << label << ": ";
    }
    std::cout << "[";
    for (size_t i = 0; i < arr.size(); i++) {
        std::cout << arr[i];
        if (i < arr.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;
}

/**
 * @brief 检查数组是否已排序
 */
bool isSorted(const std::vector<int>& arr) {
    for (size_t i = 1; i < arr.size(); i++) {
        if (arr[i] < arr[i - 1]) {
            return false;
        }
    }
    return true;
}

/**
 * @brief 测试快速排序
 */
void testQuickSort() {
    std::cout << "\n========== 快速排序测试 ==========\n" << std::endl;
    
    // 测试1: 基本测试
    {
        std::vector<int> arr = {64, 34, 25, 12, 22, 11, 90};
        printArray(arr, "原始数组");
        quickSort(arr);
        printArray(arr, "排序后");
        assert(isSorted(arr));
        std::cout << "✅ 基本测试通过\n" << std::endl;
    }
    
    // 测试2: 已排序数组
    {
        std::vector<int> arr = {1, 2, 3, 4, 5, 6, 7, 8, 9};
        printArray(arr, "已排序数组");
        quickSort(arr);
        printArray(arr, "排序后");
        assert(isSorted(arr));
        std::cout << "✅ 已排序数组测试通过\n" << std::endl;
    }
    
    // 测试3: 逆序数组
    {
        std::vector<int> arr = {9, 8, 7, 6, 5, 4, 3, 2, 1};
        printArray(arr, "逆序数组");
        quickSort(arr);
        printArray(arr, "排序后");
        assert(isSorted(arr));
        std::cout << "✅ 逆序数组测试通过\n" << std::endl;
    }
    
    // 测试4: 有重复元素
    {
        std::vector<int> arr = {5, 2, 8, 2, 9, 1, 5, 5};
        printArray(arr, "有重复元素");
        quickSort(arr);
        printArray(arr, "排序后");
        assert(isSorted(arr));
        std::cout << "✅ 重复元素测试通过\n" << std::endl;
    }
    
    // 测试5: 单元素和空数组
    {
        std::vector<int> arr1 = {42};
        quickSort(arr1);
        assert(isSorted(arr1));
        
        std::vector<int> arr2;
        quickSort(arr2);
        assert(isSorted(arr2));
        std::cout << "✅ 边界情况测试通过\n" << std::endl;
    }
    
    // 测试6: 大数组性能测试
    {
        const int size = 10000;
        std::vector<int> arr(size);
        
        // 生成随机数组
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, 100000);
        for (int& val : arr) {
            val = dis(gen);
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        quickSort(arr);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        assert(isSorted(arr));
        std::cout << "✅ 大数组测试通过 (" << size << " 个元素)" << std::endl;
        std::cout << "   排序耗时: " << duration.count() << " ms\n" << std::endl;
    }
    
    // 测试7: 三路快速排序（大量重复元素）
    {
        std::vector<int> arr = {5, 2, 5, 5, 2, 8, 2, 5, 1, 5, 5, 2};
        printArray(arr, "大量重复元素");
        quickSort3Way(arr);
        printArray(arr, "三路排序后");
        assert(isSorted(arr));
        std::cout << "✅ 三路快速排序测试通过\n" << std::endl;
    }
    
    std::cout << "========== 所有测试通过！ ==========\n" << std::endl;
}

/**
 * @brief 主函数
 */
int main() {
    std::cout << "\n╔═══════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║         快速排序算法 - C++ 实现                  ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════╝\n" << std::endl;
    
    // 运行测试
    testQuickSort();
    
    // 交互式演示
    std::cout << "========== 交互式演示 ==========\n" << std::endl;
    std::cout << "请输入要排序的整数（用空格分隔，输入回车结束）：" << std::endl;
    std::cout << "示例: 5 2 8 1 9 3\n" << std::endl;
    
    std::vector<int> userArr;
    int num;
    while (std::cin >> num) {
        userArr.push_back(num);
    }
    
    if (!userArr.empty()) {
        printArray(userArr, "你输入的数组");
        quickSort(userArr);
        printArray(userArr, "排序结果");
    }
    
    return 0;
}
