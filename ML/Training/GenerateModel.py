import math
import pandas as pd
import numpy as np
import tensorflow as tf
from sklearn.model_selection import train_test_split
import random
from everywhereml.code_generators.tensorflow import tf_porter

NUM_NN_LAYERS = 5

CSV_PATH = "../Data/ALL_DATA.csv"
MODEL_PATH = ""

if __name__ == '__main__':

    df = pd.read_csv(CSV_PATH)
    df = df.replace(
        [' NO_EVENT', ' TOUCHED', ' UNTOUCHED'],
        [      0,          1,           2     ]
    )

    # Extract data into arrays
    avgs = np.asarray(df['avg'])
    vars = np.asarray(df['var'])
    mmms = np.asarray(df['maxminusmin'])
    lmfs = np.asarray(df['lastminusfirst'])
    labels = np.asarray(df['label'])

    # Split into test and training data: 60% train, 40% test
    avgs_train, avgs_test, vars_train, vars_test, mmms_train, mmms_test, lmfs_train, lmfs_test, labels_train, labels_test = \
        [np.asarray(x).astype('float32') for x in train_test_split(avgs, vars, mmms, lmfs, labels, test_size=0.4)]

    train_data = np.asarray(list(zip(avgs_train, vars_train, mmms_train, lmfs_train)))
    test_data = np.asarray(list(zip(avgs_test, vars_test, mmms_test, lmfs_test)))

    model = tf.keras.Sequential()
    model.add(tf.keras.layers.Dense(6, activation='relu', name='a', input_shape=(4,)))
    for i in range (NUM_NN_LAYERS - 2):
        model.add(tf.keras.layers.Dense(8, activation='relu', name=f"layer_{i}"))
    model.add(tf.keras.layers.Dense(1, activation='relu'))

    model.compile(optimizer='rmsprop', loss='mse', metrics=['mae', 'accuracy'])
    model.summary()
    model.fit(train_data, labels_train, epochs=5, validation_data=(test_data, labels_test))
    print("LOSS/ACC: ", model.evaluate(test_data, labels_test))

    porter = tf_porter(model, train_data, labels_train)
    cpp_code = porter.to_cpp(instance_name="randomNN", arena_size=2048, )

    with open('MODEL2.h', 'w') as fp:
        fp.write(cpp_code)

