import unittest
from pyvideomeg import read_data


class TestFifReader(unittest.TestCase):

    def setUp(self):
        #PATH TO TEST FIF-FILE
        fname = "/home/janne/antti_data/VideoMEG/showcases/TJ/TJ_spont01R_st_mc.fif"
        self.FifData = read_data.FifData(fname)

    def test_load_timestamps_no_maxshield(self):
        timestamps = self.FifData.get_timestamps()
        self.assertNotEqual(len(timestamps), 0)

    def test_get_first_timestamp(self):
        ts = self.FifData.start_time
        self.assertIsInstance(ts, (int, float))

    def test_get_events(self):
        events = self.FifData.get_events()
        self.assertIsNotNone(events)

    def tearDown(self):
        None

if __name__ == '__main__':
    unittest.main()
