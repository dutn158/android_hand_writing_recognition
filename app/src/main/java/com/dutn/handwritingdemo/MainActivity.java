package com.dutn.handwritingdemo;

import android.content.res.AssetManager;
import android.graphics.PointF;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.dutn.handwritingdemo.views.DrawModel;
import com.dutn.handwritingdemo.views.DrawView;
import com.google.android.gms.ads.AdListener;
import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.AdView;
import com.google.android.gms.ads.MobileAds;
import com.google.gson.GsonBuilder;

import java.io.InputStream;

public class MainActivity extends AppCompatActivity implements View.OnClickListener, View.OnTouchListener {


    private static final String TAG = "MainActivity";

    // ui related
    private Button clearBtn, classBtn;
    private TextView resText;

    // views related
    private DrawModel drawModel;
    private DrawView drawView;
    private static final int PIXEL_WIDTH = 28;

    private PointF mTmpPiont = new PointF();

    private float mLastX;
    private float mLastY;

    private NeuralNetwork neuralNetwork;

    private AdView mAdView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // ads
        MobileAds.initialize(this, "ca-app-pub-2596474184327279~7884519141");
        mAdView = (AdView) findViewById(R.id.adView);
        AdRequest adRequest = new AdRequest.Builder()
                .build();
        mAdView.setAdListener(new AdListener() {
            @Override
            public void onAdClosed() {
                super.onAdClosed();
                Log.d(TAG, "onAdClosed() called");
            }

            @Override
            public void onAdFailedToLoad(int i) {
                super.onAdFailedToLoad(i);
                Log.d(TAG, "onAdFailedToLoad() called with: i = [" + i + "]");
            }

            @Override
            public void onAdLeftApplication() {
                super.onAdLeftApplication();
                Log.d(TAG, "onAdLeftApplication() called");
            }

            @Override
            public void onAdOpened() {
                super.onAdOpened();
                Log.d(TAG, "onAdOpened() called");
            }

            @Override
            public void onAdLoaded() {
                super.onAdLoaded();
                Log.d(TAG, "onAdLoaded() called");
            }
        });
        mAdView.loadAd(adRequest);

        //get drawing view
        drawView = (DrawView) findViewById(R.id.drawer_view);
        drawModel = new DrawModel(PIXEL_WIDTH, PIXEL_WIDTH);

        drawView.setModel(drawModel);
        drawView.setOnTouchListener(this);

        //clear button
        clearBtn = (Button) findViewById(R.id.btn_clear);
        clearBtn.setOnClickListener(this);

        //class button
        classBtn = (Button) findViewById(R.id.btn_class);
        classBtn.setOnClickListener(this);

        // res text
        resText = (TextView) findViewById(R.id.tfRes);

        new Thread(new Runnable() {
            @Override
            public void run() {
                AssetManager assetManager = getAssets();
                try {
                    InputStream is = assetManager.open("data/mnist_trained.json");
                    StringBuffer sb = new StringBuffer();
                    byte[] buff = new byte[is.available()];
                    int read = is.read(buff);
                    sb.append(new String(buff));
                    is.close();
                    neuralNetwork = new GsonBuilder().create().fromJson(sb.toString(), NeuralNetwork.class);
                } catch (Exception ex) {
                    ex.printStackTrace();
                }
            }
        }).start();
    }

    @Override
    protected void onResume() {
        drawView.onResume();
        // Resume the AdView.
        mAdView.resume();
        super.onResume();
    }

    @Override
    protected void onPause() {
        drawView.onPause();
        // Pause the AdView.
        mAdView.pause();
        super.onPause();
    }

    @Override
    public void onDestroy() {
        // Destroy the AdView.
        mAdView.destroy();

        super.onDestroy();
    }

    @Override
    public void onClick(View view) {
        if (view.getId() == R.id.btn_clear) {
            drawModel.clear();
            drawView.reset();
            drawView.invalidate();
            resText.setText("Result: ");
        } else if (view.getId() == R.id.btn_class) {
            float pixels[] = drawView.getPixelData();

            float[][] data = NeuralNetwork.flatten(pixels, PIXEL_WIDTH);

            if (neuralNetwork != null) {
                float[][] predicted = neuralNetwork.feedForward(data);
                float max = predicted[0][0];
                int index = 0;
                StringBuffer sb = new StringBuffer();
                sb.append(String.format("%.02f", max) + ",   ");
                for (int i = 1; i < predicted.length; i++) {
                    sb.append(String.format("%.02f", predicted[i][0]));
                    if (max < predicted[i][0]) {
                        max = predicted[i][0];
                        index = i;
                    }
                }

                Log.e(TAG, String.format("predict %d matching %.02f", index, max));
                Log.e(TAG, sb.toString());
                resText.setText(String.format("predict %d", index));
            }
        }
    }

    @Override
    public boolean onTouch(View v, MotionEvent event) {
        int action = event.getAction() & MotionEvent.ACTION_MASK;

        if (action == MotionEvent.ACTION_DOWN) {
            processTouchDown(event);
            return true;

        } else if (action == MotionEvent.ACTION_MOVE) {
            processTouchMove(event);
            return true;

        } else if (action == MotionEvent.ACTION_UP) {
            processTouchUp();
            return true;
        }
        return false;
    }

    private void processTouchDown(MotionEvent event) {
        mLastX = event.getX();
        mLastY = event.getY();
        drawView.calcPos(mLastX, mLastY, mTmpPiont);
        float lastConvX = mTmpPiont.x;
        float lastConvY = mTmpPiont.y;
        drawModel.startLine(lastConvX, lastConvY);
    }

    private void processTouchMove(MotionEvent event) {
        float x = event.getX();
        float y = event.getY();

        drawView.calcPos(x, y, mTmpPiont);
        float newConvX = mTmpPiont.x;
        float newConvY = mTmpPiont.y;
        drawModel.addLineElem(newConvX, newConvY);

        mLastX = x;
        mLastY = y;
        drawView.invalidate();
    }

    private void processTouchUp() {
        drawModel.endLine();
    }
}
