/*
 *Created by William on 2018/4/1 0001
 */
public class ArrayToStack {
    private int[] arr;
    private int size;
    public ArrayToStack(int initSize){
        if(initSize < 0){
            throw new IllegalArgumentException("init size is less than 0");
        }
        arr = new int[initSize];
        size=0;
    }
    public void push(int number){
        if(size == arr.length){
            throw new ArrayIndexOutOfBoundsException("The Stack is full");
        }
        arr[size++] = number;
    }
    public int pop(){
        if(size == 0){
            throw new ArrayIndexOutOfBoundsException("The Stack is empty");
        }
        return arr[--size];
    }
    public int peek(){
        if(size == 0){
            throw new ArrayIndexOutOfBoundsException("The Stack is empty");
        }
        return arr[size-1];
    }
}
