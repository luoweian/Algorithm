package interviewQuestions;

import java.util.Stack;

/*
 *Created by William on 2018/4/2 0002
 * 实现一个特殊的栈，在实现的基本功能的基础上，再实现返回栈中元素的最小值。
 * 要求:
 * 1.pop，push，getMin操作时间复杂度为O（1）
 * 2.设计的栈类型可以使用现成的栈结构
 */
public class GetMinStack {
    private Stack<Integer> stack;
    private Stack<Integer> help;

    public void push(int number){
        stack.push(number);
        if(help.size() > 0){
            int compareNumber = help.peek();
            help.push(compareNumber < number ? compareNumber:number);
        }else {
            help.push(number);
        }
    }
    public int pop(){
        int popNumber = stack.pop();
        help.pop();
        return popNumber;
    }
    public int getMin(){
        return help.peek();
    }

}
