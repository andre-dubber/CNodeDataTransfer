defmodule Mp3Stream.Encoder do

  @node "c1@"
  @file_chunk_size 8 * 1024

  def mp3(filename) do
    call_cnode {:mp3, filename}
  end

  def encode(path) do
    true = File.exists?(path)
    {:ok, encoded_file} = File.open("/tmp/result.mp3", [:write])
    File.open(path, [:read], fn(fid) ->
      pipe_file(fid, encoded_file)
    end)
  end

  def detect_wav_parameters(file, <<"RIFF", filesize::little-integer-size(32),
    "WAVEfmt ",
    _format_len::little-integer-size(32),
    _pcm_format::little-integer-size(16),
    channels::little-integer-size(16),
    sample_rate::little-integer-size(32),
    bytes_per_second::little-integer-size(32),
    _block_align::little-integer-size(16),
    bits_per_sample::little-integer-size(16),
    "data",
    contents::binary>>) do
    {file, %{channels: channels, filesize: filesize, bytes_per_second: bytes_per_second, sample_rate: sample_rate, bits_per_sample: bits_per_sample, contents: contents}}
  end
  def detect_wav_parameters(file, <<"RIFF", filesize::little-integer-size(32),
    "WAVEfmt ",
    _format_len::little-integer-size(32),
    _pcm_format::little-integer-size(16),
    channels::little-integer-size(16),
    sample_rate::little-integer-size(32),
    bytes_per_second::little-integer-size(32),
    _block_align::little-integer-size(16),
    bits_per_sample::little-integer-size(16),
    "LIST",
    info_size::little-integer-size(16),
    contents::binary>>) do
    #IO.puts("Leftover from initial read #{inspect contents}")
    #IO.puts("Information block size detected #{inspect info_size}")
    #<<list_chunk::binary-size(info_size), "data">> = IO.binread(file, info_size + 4)
    extra_header = contents <> IO.binread(file, info_size+4)
    list_header_size = info_size + 2
    #IO.puts("list header size detected #{inspect list_header_size}")
    #IO.puts("Read extra header size #{inspect byte_size(extra_header)} #{inspect Base.encode16(extra_header)}")
    <<list_chunk::binary-size(list_header_size), "data">> = extra_header
    #IO.puts("Data after information block  #{inspect extra_contents}")


    IO.puts("Data from list block #{to_string list_chunk}")
    {file, %{channels: channels, filesize: filesize, bytes_per_second: bytes_per_second, sample_rate: sample_rate, bits_per_sample: bits_per_sample, contents: contents}}
  end
#<<"RIFF", filesize::little-integer-size(32),"WAVEfmt ",format_len::little-integer-size(32),pcm_format::little-integer-size(16),channels::little-integer-size(16),sample_rate::little-integer-size(32),bytes_per_second::little-integer-size(32),_block_align::little-integer-size(16),bits_per_sample::little-integer-size(16), "LIST", extra_size::little-integer-size(16),list_chunk::byte-size(114+2), "data", contents::binary>> = header
  def detect_wav_parameters(file, invalid_data) do
    require Logger
    Logger.error("Unrecognzied wav file format, header #{inspect invalid_data}")
  end

  #Mp3Stream.Encoder.encode2("/tmp/sample_stereo.wav")

  def encode2(path) do
    true = File.exists?(path)
    {:ok, encoded_file} = File.open("/tmp/result.mp3", [:binary, :write, :raw])

    Stream.resource(
      fn ->
        file = File.open!(path)
        header = IO.binread(file, 44)
        detect_wav_parameters(file, header)
      end,
      fn {file, stats} ->
        case IO.binread(file, @file_chunk_size) do
          data when is_binary(data) -> {[{stats, data}], {file, stats}}
          _ -> {:halt, file}
        end
      end,
      fn file -> File.close(file) end
    )
    |> Stream.with_index
    |> Stream.each(fn( {{stats, chunk}, index} ) ->
      #IO.puts("Each called #{index} time with with #{inspect stats}, #{inspect chunk}")
      #IO.puts("Stats #{inspect stats}")
      log_encode({stats, chunk, index}, encoded_file)
    end)
    |> Stream.run

  end

  def log_encode({stats, chunk, chunk_no}, file ) do
    #IO.puts("Got chunk #{byte_size chunk} bytes, no #{inspect chunk_no}")
    case chunk_no < 5 do
      true -> File.open!("/tmp/exnode#{inspect chunk_no}.wav", [:binary, :write]) |> IO.binwrite(chunk) |> File.close
      _ ->
    end
    encoded = call_cnode {:encode, chunk, stats.channels}
    #encoded = chunk
    :ok = IO.binwrite(file, encoded)
  end

  defp pipe_file(fid, encoded_file) do
    # we read files in blocks to avoid excessive memory usage
    Stream.repeatedly(fn -> :file.binread(fid, @file_chunk_size) end)
    |> Stream.take_while(fn
      :eof        -> false
      {:error, _} -> false
      _           -> true
    end)
    |> Stream.map(fn {:ok, data} -> data end)
    |> stream_to_cnode(encoded_file)

  end

  defp stream_to_cnode(enum, encoded_file) do
    Enum.each(enum, fn(iodata) ->
      encoded = call_cnode {:encode, iodata}
      :ok = IO.binwrite(encoded_file, encoded)
    end)
    File.close(encoded_file)
  end

  defp call_cnode(msg) do
    #IO.puts("Calculated address: #{@node<>to_string(hostname)} ")
    #IO.puts("Calling cnode with : #{inspect msg} ")
    send_res = send {:any, String.to_atom(@node<>to_string(hostname))}, {:call, self, msg}
    #IO.puts("Send returned #{inspect send_res}")
    receive do
      {:cnode, result} ->
        result
    after
      10_000 -> IO.puts("Failed after 10s")
    end
  end

  defp hostname do
    {:ok, hostname} = :inet.gethostname
    hostname
  end
end




