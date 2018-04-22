package dynamicProgramming;

/*
 *Created by William on 2018/4/21 0021
 * 给定一个二维数组，求到达最右下角
 */
public class MinRouteToRightConer {
    public static int minValue(int[][] matrix, int i, int j) {
        if (i == matrix.length - 1 && j == matrix[0].length - 1) {
            return matrix[i][j];
        }
        if (i == matrix.length - 1) {
            return matrix[i][j] + minValue(matrix, i, j + 1);
        }
        if (j == matrix[0].length - 1) {
            return matrix[i][j] + minValue(matrix, i + 1, j);
        } else {
            return matrix[i][j] + Math.min(minValue(matrix, i, j + 1), minValue(matrix, i + 1, j));
        }
    }
}
