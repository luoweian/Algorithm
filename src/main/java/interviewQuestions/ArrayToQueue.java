package interviewQuestions;

/*
 *Created by William on 2018/4/2 0002
 */
public class ArrayToQueue {
    private int[] arr;
    private int start;
    private int end;
    private int size;
    ArrayToQueue(int queueSize){
        if(queueSize <= 0){
            throw new ExceptionInInitializerError("queue size must bigger than 0");
        }
        arr = new int[queueSize];
        size = start = end =0;
    }
    public void push(int number){
        if(size == arr.length){
            throw new ArrayIndexOutOfBoundsException("the queue is out of bounds");
        }
        arr[end] = number;
        size++;
        end = end==arr.length-1 ? 0 : end+1;
    }
    public int poll(){
        if(size == 0){
            throw new ArrayIndexOutOfBoundsException("the queue is empty");
        }
        int returnNumber = arr[start];
        start = start == arr.length-1 ? 0 : start+1;
        size--;
        return returnNumber;
    }
    public int peek(){
        if(size == 0){
            throw new ArrayIndexOutOfBoundsException("the queue is empty");
        }
        return arr[start];
    }
}
