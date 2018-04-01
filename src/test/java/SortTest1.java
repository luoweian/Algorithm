import org.junit.Test;

/*
 *Created by William on 2018/3/31 0031
 */
public class SortTest1 {
    @Test
    public void test(){
        int[] arr = {1,6,-5,123,411,-55};
        //System.out.println(getMax(arr,0,arr.length-1));
        arrBiggerThanNumber(arr,6);
        for(int ar:arr){
            System.out.print(ar+",");
        }
    }
    public static int getMax(int[] arr, int l, int r) {
        if (l == r) {
            return arr[l];
        }
        int mid = l + ((r - l) >> 1);	//mid = (l + r) / 2
        int maxLeft = getMax(arr,l, mid);
        int maxRight = getMax(arr, mid+1, r);
        return Math.max(maxLeft, maxRight);
    }
    public static void arrBiggerThanNumber(int[] arr, int number){
        int x = -1;
        int g = arr.length;
        for(int i =0;i<arr.length;i++){
            if(arr[i] < number){
                swap(arr, ++x, i);
            }

        }
    }

    public static void swap(int[] arr, int i, int j){
        int y = arr[i];
        arr[i] = arr[j];
        arr[j] = y;
    }

    public boolean addStack(int number){
        if(index >= arr.length){
            return false;
        }
        arr[index] = number;
        index++;
        return true;
    }

    private int[] arr = new int[10];
    private int index = 0;
}
