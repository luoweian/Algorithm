package trieTreeAndGreedyAlgorithm;

import java.util.Comparator;
import java.util.PriorityQueue;

/**
 * 一块金条切成两半，需要花费和长度数值一样的铜板的。比如
 * 长度为20的金条，不管切成长度多大的两块，都需要花费20个
 * 铜板。一圈人想整分整块金条，怎么分最省铜板？
 * 例如，给定数组{10,20,30}，代表一共三个人，整块金条长度
 * 为10+20+30=60.金条要分成10，20，30三个部分。如果，先把
 * 长度60的金条分成10和50，花费60再把长度50的金条分成20和
 * 30，需要花费50个铜板，一个花费110铜板。
 * 但是如果，先把长度60的金条分成30和30，花费60，再把长度30
 * 金条分成10和20，花费30，一个花费90个铜板。
 * 输入一个数组，返回分割的最小代价。
 */
public class Code_02_Less_Money {

	public static int lessMoney(int[] arr) {
		PriorityQueue<Integer> pQ = new PriorityQueue<>();
		for (int i = 0; i < arr.length; i++) {
			pQ.add(arr[i]);
		}
		int sum = 0;
		int cur = 0;
		while (pQ.size() > 1) {
			cur = pQ.poll() + pQ.poll();
			sum += cur;
			pQ.add(cur);
		}
		return sum;
	}

	public static class MinheapComparator implements Comparator<Integer> {

		@Override
		public int compare(Integer o1, Integer o2) {
			return o1 - o2; // < 0  o1 < o2  负数
		}

	}

	public static class MaxheapComparator implements Comparator<Integer> {

		@Override
		public int compare(Integer o1, Integer o2) {
			return o2 - o1; // <   o2 < o1
		}

	}

	public static void main(String[] args) {
		// solution
		int[] arr = { 6, 7, 8, 9 };
		System.out.println(lessMoney(arr));

		int[] arrForHeap = { 3, 5, 2, 7, 0, 1, 6, 4 };

		// min heap
		PriorityQueue<Integer> minQ1 = new PriorityQueue<>();
		for (int i = 0; i < arrForHeap.length; i++) {
			minQ1.add(arrForHeap[i]);
		}
		while (!minQ1.isEmpty()) {
			System.out.print(minQ1.poll() + " ");
		}
		System.out.println();

		// min heap use Comparator
		PriorityQueue<Integer> minQ2 = new PriorityQueue<>(new MinheapComparator());
		for (int i = 0; i < arrForHeap.length; i++) {
			minQ2.add(arrForHeap[i]);
		}
		while (!minQ2.isEmpty()) {
			System.out.print(minQ2.poll() + " ");
		}
		System.out.println();

		// max heap use Comparator
		PriorityQueue<Integer> maxQ = new PriorityQueue<>(new MaxheapComparator());
		for (int i = 0; i < arrForHeap.length; i++) {
			maxQ.add(arrForHeap[i]);
		}
		while (!maxQ.isEmpty()) {
			System.out.print(maxQ.poll() + " ");
		}

	}

}
