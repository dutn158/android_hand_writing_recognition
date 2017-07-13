package com.dutn.handwritingdemo;

import com.google.gson.annotations.SerializedName;

public class NeuralNetwork {

    @SerializedName("sizes")
    public int[] sizes;

    @SerializedName("weights")
    public float[][][] weights;

    @SerializedName("biases")
    public float[][][] biases;

    public static float[][] sigmoid(float[][] x) {
        float[][] res = new float[x.length][x[0].length];
        for (int i = 0; i < x.length; i++) {
            for (int j = 0; j < x[0].length; j++) {
                res[i][j] = (float) (1.0 / (1.0 + Math.exp((double) -x[i][j])));
            }
        }
        return res;
    }

    public float[][] feedForward(float[][] input) {
        float[][] w1 = weights[0];
        float[][] b1 = biases[0];
        float[][] w2 = weights[1];
        float[][] b2 = biases[1];

        float[][] l1 = sigmoid(add(dot(w1, input), b1));
        float[][] l2 = sigmoid(add(dot(w2, l1), b2));

        return l2;
    }

    public static float[][] dot(float[][] a, float[][] b) {
        int n = a.length;
        int m = b[0].length;
        int k = a[0].length;
        float[][] res = new float[n][m];
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < m; j++) {
                float dp = 0;
                for (int ki = 0; ki < k; ki++) {
                    dp += a[i][ki] * b[ki][j];
                }
                res[i][j] = dp;
            }
        }
        return res;
    }

    public static float[][] add(float[][] a, float[][] b) {
        int n = a.length;
        int m = a[0].length;
        float[][] res = new float[n][m];
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < m; j++) {
                res[i][j] = a[i][j] + b[i][j];
            }
        }
        return res;
    }

    public static float[][] flatten(float[] data, int width) {
        float[][] res = new float[width * width][1];

        for (int i = 0; i < width; i++) {
            for (int j = 0; j < width; j++) {
                res[(i * width) + j][0] = data[(i * width) + j];
            }
        }

        return res;
    }

    public static void main(String[] args) {
        float[][] a = {{1, 2, 3},
                {1, 2, 3}};
        float[][] b = {{1}, {2}, {3}};

        float[][] c = dot(a, b);
        System.out.print(c);
    }

}
