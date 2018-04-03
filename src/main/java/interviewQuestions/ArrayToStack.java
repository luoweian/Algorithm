package interviewQuestions;

/*
 *Created by William on 2018/4/2 0002
 * 数组实现栈
 */
public class ArrayToStack {
    private int[] arr;
    private int index;
    public ArrayToStack(int stackSize){
        if(stackSize <=0){
            throw new ExceptionInInitializerError("stackSize must bigger than 0");
        }
        arr = new int[stackSize];
        index = 0;
    }

    public void push(int number){
        if(index == arr.length){
            throw new ArrayIndexOutOfBoundsException("the stack is full");
        }
        arr[index++] = number;
    }
    public int pop(){
        if(index == 0){
            throw new IllegalArgumentException("the stack is empty");
        }
        return arr[--index];
    }
    public int peek(){
        if(index == 0){
            throw new IllegalArgumentException("the stack is empty");
        }
        return arr[index-1];
    }
}
