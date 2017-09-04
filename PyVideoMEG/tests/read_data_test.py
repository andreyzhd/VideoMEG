import unittest
from os import path as op
from pyvideomeg import read_data


class TestFifReader(unittest.TestCase):

    def setUp(self):
        #PATH TO TEST FIF-FILE
        fname = ""
        self.FifData = read_data.FifData(fname, 'STI 006')

    def test_load_timestamps_no_maxshield(self):
        timestamps = self.FifData.get_timestamps()
        self.assertNotEqual(len(timestamps), 0)

    def test_get_first_timestamp(self):
        ts = self.FifData.start_time
        self.assertIsInstance(ts, (int, float))

    def tearDown(self):
        pass

class TestEvlReader(unittest.TestCase):
    def setUp(self):
        fname = op.join(op.dirname(__file__), "test_evl.evl")
        print("Reading EVL")
        self.EvlData = read_data.EvlData.from_file(fname)

    def test_events(self):
        # Start and end should NOT be included
        self.assertEqual(len(self.EvlData.get_events()), 2)

    def test_start(self):
        self.assertAlmostEqual(self.EvlData.rec_start, 77.919, 2)

    def tearDown(self):
        pass


if __name__ == '__main__':
    unittest.main()
