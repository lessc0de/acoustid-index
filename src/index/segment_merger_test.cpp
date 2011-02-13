// Acoustid Index -- Inverted index for audio fingerprints
// Copyright (C) 2011  Lukas Lalinsky
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <gtest/gtest.h>
#include <QFile>
#include "util/test_utils.h"
#include "store/ram_directory.h"
#include "store/input_stream.h"
#include "store/output_stream.h"
#include "segment_data_reader.h"
#include "segment_data_writer.h"
#include "segment_index.h"
#include "segment_index_reader.h"
#include "segment_index_writer.h"
#include "segment_enum.h"
#include "segment_merger.h"

using namespace Acoustid;

TEST(SegmentMergerTest, Iterate)
{
	RAMDirectory dir;

	{
		ScopedPtr<OutputStream> indexOutput(dir.createFile("segment_0.fii"));
		SegmentIndexWriter indexWriter(indexOutput.get());
		indexWriter.setBlockSize(8);

		ScopedPtr<OutputStream> dataOutput(dir.createFile("segment_0.fid"));
		SegmentDataWriter writer(dataOutput.get(), &indexWriter, indexWriter.blockSize());
		writer.addItem(200, 300);
		writer.addItem(201, 301);
		writer.addItem(201, 302);
		writer.addItem(202, 303);
		writer.close();
	}

	{
		ScopedPtr<OutputStream> indexOutput(dir.createFile("segment_1.fii"));
		SegmentIndexWriter indexWriter(indexOutput.get());
		indexWriter.setBlockSize(8);

		ScopedPtr<OutputStream> dataOutput(dir.createFile("segment_1.fid"));
		SegmentDataWriter writer(dataOutput.get(), &indexWriter, indexWriter.blockSize());
		writer.addItem(199, 500);
		writer.addItem(201, 300);
		writer.addItem(201, 304);
		writer.addItem(202, 303);
		writer.addItem(500, 501);
		writer.close();
	}

	{
		ScopedPtr<OutputStream> indexOutput(dir.createFile("segment_2.fii"));
		SegmentIndexWriter indexWriter(indexOutput.get());
		indexWriter.setBlockSize(8);

		ScopedPtr<OutputStream> dataOutput(dir.createFile("segment_2.fid"));
		SegmentDataWriter writer(dataOutput.get(), &indexWriter, indexWriter.blockSize());

		ScopedPtr<InputStream> indexInput1(dir.openFile("segment_0.fii"));
		ScopedPtr<InputStream> dataInput1(dir.openFile("segment_0.fid"));
		ScopedPtr<SegmentIndex> index1(SegmentIndexReader(indexInput1.get()).read());
		SegmentDataReader dataReader1(dataInput1.get(), index1->blockSize());
		SegmentEnum reader1(index1.get(), &dataReader1);

		ScopedPtr<InputStream> indexInput2(dir.openFile("segment_1.fii"));
		ScopedPtr<InputStream> dataInput2(dir.openFile("segment_1.fid"));
		ScopedPtr<SegmentIndex> index2(SegmentIndexReader(indexInput2.get()).read());
		SegmentDataReader dataReader2(dataInput2.get(), index2->blockSize());
		SegmentEnum reader2(index2.get(), &dataReader2);

		SegmentMerger merger(&writer);
		merger.addSource(&reader1);
		merger.addSource(&reader2);
		merger.merge();
	}

	ScopedPtr<InputStream> indexInput(dir.openFile("segment_2.fii"));
	ScopedPtr<InputStream> dataInput(dir.openFile("segment_2.fid"));
	ScopedPtr<SegmentIndex> index(SegmentIndexReader(indexInput.get()).read());
	SegmentDataReader dataReader(dataInput.get(), index->blockSize());
	SegmentEnum reader(index.get(), &dataReader);

	ASSERT_TRUE(reader.next());
	ASSERT_EQ(199, reader.key());
	ASSERT_EQ(500, reader.value());
	ASSERT_TRUE(reader.next());
	ASSERT_EQ(200, reader.key());
	ASSERT_EQ(300, reader.value());
	ASSERT_TRUE(reader.next());
	ASSERT_EQ(201, reader.key());
	ASSERT_EQ(300, reader.value());
	ASSERT_TRUE(reader.next());
	ASSERT_EQ(201, reader.key());
	ASSERT_EQ(301, reader.value());
	ASSERT_TRUE(reader.next());
	ASSERT_EQ(201, reader.key());
	ASSERT_EQ(302, reader.value());
	ASSERT_TRUE(reader.next());
	ASSERT_EQ(201, reader.key());
	ASSERT_EQ(304, reader.value());
	ASSERT_TRUE(reader.next());
	ASSERT_EQ(202, reader.key());
	ASSERT_EQ(303, reader.value());
	ASSERT_TRUE(reader.next());
	ASSERT_EQ(500, reader.key());
	ASSERT_EQ(501, reader.value());
	ASSERT_FALSE(reader.next());
}
