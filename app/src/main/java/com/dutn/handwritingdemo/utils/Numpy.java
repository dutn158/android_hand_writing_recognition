package com.dutn.handwritingdemo.utils;

public class Numpy {

    static {
        System.loadLibrary("numpy");
    }

    public static native double[][] dot(double[][] v, double[][] w);

    public static native double[][] add(double[][] v, double[][] w);

}
