#include "arg.h"
#include "common.h"
#include "log.h"
#include "llama.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>

std::string urlEncode(const std::string &value);
std::string urlDecode(const std::string &value);

static void print_usage(int argc, char ** argv) {
    LOG("\nexample usage:\n");
    LOG("\n    %s -m model.gguf -n 32 -np 4\n", argv[0]);
    LOG("\n");
}

std::string urlEncode(const std::string &value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (char c : value) {
        // Keep alphanumeric and other accepted characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << std::uppercase;
        escaped << '%' << std::setw(2) << int((unsigned char)c);
        escaped << std::nouppercase;
    }

    return escaped.str();
}

std::string urlDecode(const std::string &value) {
    std::string result;
    result.reserve(value.length());

    for (std::size_t i = 0; i < value.length(); ++i) {
        if (value[i] == '%') {
            if (i + 2 < value.length()) {
                int hex_val;
                std::istringstream hex_stream(value.substr(i + 1, 2));
                if (hex_stream >> std::hex >> hex_val) {
                    result += static_cast<char>(hex_val);
                    i += 2;
                } else {
                    result += '%';
                }
            } else {
                result += '%';
            }
        } else if (value[i] == '+') {
            result += ' ';
        } else {
            result += value[i];
        }
    }

    return result;
}

int main(int argc, char ** argv) {
    common_params params;

    params.n_predict = 64;
    params.n_ctx = 4096; // Default context size

    if (!common_params_parse(argc, argv, params, LLAMA_EXAMPLE_COMMON, print_usage)) {
        return 1;
    }

    common_init();

    int n_parallel = params.n_parallel;
    int n_predict = params.n_predict;

    llama_backend_init();
    llama_numa_init(params.numa);

    LOG("LOADING\n");

    llama_model_params model_params = common_model_params_to_llama(params);
    llama_model * model = llama_load_model_from_file(params.model.c_str(), model_params);

    if (model == NULL) {
        fprintf(stderr , "%s: error: unable to load model\n" , __func__);
        return 1;
    }

    // Initialize a single, large context
    llama_context_params ctx_params = common_context_params_to_llama(params);
    ctx_params.n_ctx = params.n_ctx;
    ctx_params.n_batch = params.n_ctx;

    llama_context * ctx = llama_new_context_with_model(model, ctx_params);

    if (ctx == NULL) {
        fprintf(stderr , "%s: error: failed to create the llama_context\n" , __func__);
        return 1;
    }
    const int n_ctx = llama_n_ctx(ctx);

    auto sparams = llama_sampler_chain_default_params();
    sparams.no_perf = false;

    llama_sampler * smpl = llama_sampler_chain_init(sparams);

    llama_sampler_chain_add(smpl, llama_sampler_init_top_k(params.sampling.top_k));
    llama_sampler_chain_add(smpl, llama_sampler_init_top_p(params.sampling.top_p, params.sampling.min_keep));
    llama_sampler_chain_add(smpl, llama_sampler_init_temp (params.sampling.temp));
    llama_sampler_chain_add(smpl, llama_sampler_init_dist (params.sampling.seed));

    std::vector<llama_token> previous_tokens_list;

    // Initialize a single, large batch
    llama_batch batch = llama_batch_init(n_ctx, 0, n_parallel);

    // Common to all sequences
    std::vector<llama_seq_id> common_seq_ids(n_parallel, 0);
    for (int32_t i = 0; i < n_parallel; ++i) { common_seq_ids[i] = i; }

    std::string input_line;
    while (true) {
        LOG("INPUT:\n");
        fflush(stderr);
        std::getline(std::cin, input_line);

        if (input_line.empty()) { break; }
        std::string prompt = urlDecode(input_line);

        std::vector<llama_token> tokens_list = common_tokenize(model, prompt, true);

        // Find overlap between new and previous tokens
        // Note the -1 here, if we dont decode at least 1 token we end up with nothing to sample in the main loop.
        size_t common_length = 0;
        for (size_t i = 0; i < std::min(previous_tokens_list.size(), tokens_list.size()-1); ++i) {
            if (previous_tokens_list[i] == tokens_list[i]) {
                common_length++;
            } else {
                break;
            }
        }

        const uint32_t n_kv_req = tokens_list.size() + n_predict*n_parallel;
        LOG("\nDEBUG: common_length = %ld, prompt_length = %ld, n_predict = %d, n_ctx = %d, n_batch = %u, n_parallel = %d, n_kv_req = %d\n", common_length, tokens_list.size(), n_predict, ctx_params.n_ctx, ctx_params.n_batch, n_parallel, n_kv_req);
        if (n_kv_req > ctx_params.n_ctx) {
            LOG("%s: error: n_kv_req (%d) > n_ctx, the required KV cache size is not big enough\n", __func__,  n_kv_req);
            LOG("%s:        either reduce n_parallel or increase n_ctx\n", __func__);
            break;
        }

        // print the prompt token-by-token
        fprintf(stderr, "\n");
        for (auto id : tokens_list) {
            fprintf(stderr, "%d(%s)", id, common_token_to_piece(ctx, id).c_str());
        }
        fflush(stderr);

        // Rewind KV caches
        for (int32_t i = 0; i < n_parallel; ++i) { 
            int32_t p0 = (common_length > 0) ? common_length : -1;
            if (!llama_kv_cache_seq_rm(ctx, i, p0, -1)) {
                LOG("%s: llama_kv_cache_seq_rm() failed seq_id=%d\n", __func__, i);
                break;
            }
        }

        // Add only the new tokens to the batch
        common_batch_clear(batch);
        for (size_t i = common_length; i < tokens_list.size(); ++i) {
            common_batch_add(batch, tokens_list[i], i, common_seq_ids, true);
        }

        if (llama_model_has_encoder(model)) {
            if (llama_encode(ctx, batch)) {
                LOG("%s : failed to eval\n", __func__);
                return 1;
            }

            llama_token decoder_start_token_id = llama_model_decoder_start_token(model);
            if (decoder_start_token_id == -1) {
                decoder_start_token_id = llama_token_bos(model);
            }

            common_batch_clear(batch);
            common_batch_add(batch, decoder_start_token_id, 0, common_seq_ids, false);
        }
        
        // Decode new tokens.
        if (llama_decode(ctx, batch) != 0) {
            LOG("%s: llama_decode() failed\n", __func__);
            break;
        }

        // assign the system KV cache to all parallel sequences
        // this way, the parallel sequences will "reuse" the prompt tokens without having to copy them
        // for (int32_t i = 1; i < n_parallel; ++i) {
        //     llama_kv_cache_seq_rm(ctx, i, -1, -1);
        //     llama_kv_cache_seq_cp(ctx, 0, i, -1, -1);
        // }

        if (n_parallel > 1) {
            LOG("\n\n%s: generating %d sequences ...\n", __func__, n_parallel);
        }
        LOG("START:%d\n", n_parallel);
        LOG("PROMPT:%s\n", urlEncode(prompt).c_str());

        // main loop

        // we will store the parallel decoded sequences in this vector
        std::vector<std::string> streams(n_parallel);

        // remember the batch index of the last token for each parallel sequence
        // we need this to determine which logits to sample from
        std::vector<int32_t> i_batch(n_parallel, batch.n_tokens - 1);

        int n_cur    = tokens_list.size();
        int n_decode = 0;
        int n_tokens = 0;

        const auto t_main_start = ggml_time_us();

        while (n_tokens <= n_predict) {
            //LOG_TEE("DEBUG: n_tokens = %d, n_cur = %d\n", n_tokens, n_cur);

            // prepare the next batch
            common_batch_clear(batch);

            // sample the next token for each parallel sequence / stream
            for (int32_t i = 0; i < n_parallel; ++i) {
                if (i_batch[i] < 0) {
                    // the stream has already finished
                    continue;
                }

                const llama_token new_token_id = llama_sampler_sample(smpl, ctx, i_batch[i]);

                // Check if the new token contains a period
                std::string new_piece = common_token_to_piece(ctx, new_token_id);
                LOG("STREAM%d:%s\n", i, urlEncode(new_piece).c_str());
                fflush(stdout);

                bool contains_period = (new_piece.find('.') != std::string::npos) || (new_piece.find('\n') != std::string::npos);

                // is it an end of generation? -> mark the stream as finished
                if (llama_token_is_eog(model, new_token_id) || n_tokens == n_predict || (contains_period && n_tokens > 2)) {
                    i_batch[i] = -1;
                    LOG("STOP%d:%d:%d\n", i, n_tokens, contains_period?1:0);
                    if (contains_period) {
                        streams[i] += new_piece;
                    }
                    continue;
                }

                streams[i] += common_token_to_piece(ctx, new_token_id);
                i_batch[i] = batch.n_tokens;

                // push this new token for next evaluation
                common_batch_add(batch, new_token_id, n_cur, { i }, true);

                n_decode += 1;
            }

            // all streams are finished
            if (batch.n_tokens == 0) {
                break;
            }

            n_cur += 1;
            n_tokens += 1;

            // evaluate the current batch with the transformer model
            if (llama_decode(ctx, batch)) {
                fprintf(stderr, "%s : failed to eval, return code %d\n", __func__, 1);
                break;
            }
        }

        LOG("\n");

        if (n_parallel > 1) {
            LOG("\n");
            for (int32_t i = 0; i < n_parallel; ++i) {
                LOG("SEQUENCE%d:%s\n", i, urlEncode(streams[i]).c_str());
            }
        }

        const auto t_main_end = ggml_time_us();

        LOG("DONE:%d\n", n_parallel);

        LOG("%s: decoded total of %d tokens from %d streams in %.2f s, speed: %.2f t/s\n",
                __func__, n_decode, n_parallel, (t_main_end - t_main_start) / 1000000.0f, n_decode / ((t_main_end - t_main_start) / 1000000.0f));

        llama_perf_context_print(ctx);
        llama_perf_context_reset(ctx);

        previous_tokens_list = tokens_list;
    }

    // Free only when exiting
    llama_batch_free(batch);
    llama_sampler_free(smpl);    
    llama_free(ctx);
    llama_free_model(model);
    llama_backend_free();

    return 0;
}
