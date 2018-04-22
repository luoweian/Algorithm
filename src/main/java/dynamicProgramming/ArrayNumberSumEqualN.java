package dynamicProgramming;

/*
 *Created by William on 2018/4/21 0021
 * 给你一个数组arr，和一个整数aim。如果可以任意选择arr中的数字，能不能累加得到aim，返回true或false
 * 都为正数
 */
public class ArrayNumberSumEqualN {
    public static boolean isSum(int[] arr, int i, int sum, int aim) {
        if (i == arr.length) {
            return sum == aim;
        }
        System.out.println(sum);
        boolean situationOne = isSum(arr, i + 1, sum, aim);
        boolean situationTwo = isSum(arr, i + 1, sum + arr[i], aim);
        return situationOne || situationTwo;
    }


    public static void main(String[] args){
        int[] arr = {1,2,3,5,4,5,6,8};
        int aim = 12;
        System.out.println(isSum(arr,0,0,aim));
    }
}
