package com.dutn.handwritingdemo.utils;


import android.graphics.Bitmap;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

public class ImageGraphProcessing {

    public static int EDGE_DETECT_KERNEL[][] = {
            {0, 1, 0},
            {1, -4, 1},
            {0, 1, 0}
    };

    public static int SHARPEN[][] = {
            {0, -1, 0},
            {-1, 5, -1},
            {0, -1, 0}
    };

    static {
        System.loadLibrary("image-graph-processing");
    }

    // Image processing
    public static native void convertToGrayScale(Bitmap src);

    public static native void convertToBinary(Bitmap src);

    public static native void setContrast(Bitmap src, int value);

    public static native void applyKernel(Bitmap src, int[][] kernel);

    // Graph processing
    public static native ArrayList<Point> getVertexes(Bitmap src);

    public static native ArrayList<ArrayList<Point>> connectedComponents(ArrayList<Point> points);

    public static native Rect getRect(ArrayList<Point> points, int n);

    //
    public static ArrayList<ArrayList<Point>> getConnectedComponents(ArrayList<ArrayList<Point>> components, int limit) {
        Collections.sort(components, new Comparator<List<Point>>() {
            @Override
            public int compare(List<Point> o1, List<Point> o2) {
                return o1.size() - o2.size();
            }
        });
        int n = components.size();
        ArrayList<ArrayList<Point>> res = new ArrayList<>();
        for (int i = n - limit; i < n; i++) {
            res.add(components.get(i));
        }
        return res;
    }

    public static Bitmap cropBitmap(Bitmap src, Rect rect) {
        return Bitmap.createBitmap(src, rect.left - 2, rect.top - 2, rect.right - rect.left + 4, rect.bottom - rect.top + 4);
    }
}
