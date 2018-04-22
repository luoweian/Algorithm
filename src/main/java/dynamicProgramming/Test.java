package dynamicProgramming;

public class Test {

	public static int f(int N) {
		if(N == 1 || N ==2) {
			return 1;
		}
		return f(N-1) + f(N-2);
	}

	//打印全部子序列
	public static void printSub(char[] str, int i,String res){
		if(i == str.length){
			System.out.println(res);
			return;
		}
		printSub(str,i+1,res);
		printSub(str,i+1,res+String.valueOf(str[i]));
	}

	private int number1=0;
	private int number2=0;
	public int printNumberOfCows(int number){
		if(number <= 4 ){
			return number;
		}
		return printNumberOfCows(number-1)+printNumberOfCows(number-3);
	}

	public static void main(String[] args){
		String test = "abc";
//		printSub(test.toCharArray(),0,"");
		Test t = new Test();
		System.out.println(t.printNumberOfCows(5));
	}
}
