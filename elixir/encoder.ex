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

  def encode2(path) do
    true = File.exists?(path)
    {:ok, encoded_file} = File.open("/tmp/result.mp3", [:binary, :write, :raw])
    File.stream!(path, [:raw, :read_ahead, :binary], @file_chunk_size)
    |> Stream.with_index
    |> Stream.each(fn(chunk) ->
        log_encode(chunk, encoded_file)
      end)
    |> Stream.run
    File.close(encoded_file)
  end

  def log_encode({chunk, chunk_no}, file ) do
    #IO.puts("Got chunk #{byte_size chunk} bytes, no #{inspect chunk_no}")
    case chunk_no < 5 do
      true -> File.open!("/tmp/3_i#{inspect chunk_no}.wav", [:binary, :write]) |> IO.binwrite(chunk) |> File.close
      _ -> 
    end
    encoded = call_cnode {:encode, chunk}
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



