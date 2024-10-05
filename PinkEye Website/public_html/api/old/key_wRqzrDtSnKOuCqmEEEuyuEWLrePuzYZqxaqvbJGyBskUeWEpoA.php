<?php
        $rate_limit = array(
            'time_interval' => 60,
            'max_requests' => 7,
            'clear_duration' => 3600,
        );
        
        $user_ip = $_SERVER['REMOTE_ADDR'];
        $current_time = time();
        
        $data_file = '../../rate_limit_data.txt';
        $data = file_get_contents($data_file);
        $rate_limit_data = array();
        if ($data !== false)
        {
            $rate_limit_data = unserialize($data);
        }
        
        if (!isset($rate_limit_data[$user_ip]))
        {
            $rate_limit_data[$user_ip] = array(
                'last_request_time' => 0,
                'request_count' => 0,
                'clear_time' => $current_time + $rate_limit['clear_duration'],
            );
        }
        
        $last_request_time = $rate_limit_data[$user_ip]['last_request_time'];
        $request_count = $rate_limit_data[$user_ip]['request_count'];
        $clear_time = $rate_limit_data[$user_ip]['clear_time'];
        
        if ($current_time >= $clear_time)
        {
            $request_count = 0;
            $clear_time = $current_time + $rate_limit['clear_duration'];
        }
        if ($current_time - $last_request_time > $rate_limit['time_interval'])
        {
            $request_count = 0;
        }
        
        $rate_limit_data[$user_ip]['last_request_time'] = $current_time;
        $rate_limit_data[$user_ip]['request_count'] = $request_count + 1;
        $rate_limit_data[$user_ip]['clear_time'] = $clear_time;
        
        file_put_contents($data_file, serialize($rate_limit_data));
        
        if ($request_count >= $rate_limit['max_requests'])
        {
            header('HTTP/1.1 429 Too Many Requests');
            header('Content-Type: text/html');
            exit();
        }
        
        if ($_SERVER['REQUEST_METHOD'] === 'GET')
        {
			if (isset($_GET['key']))
			{
			    $keyValue = $_GET['key'];
			    $found_key = false;
			    
			    $url = "https://backend-services.stand.sh/internal_get_privilege/?0=" . $keyValue;
			    $response = file_get_contents($url);
			    
			    $url_pk = "https://backend-services.stand.sh/internal_check_pinkeye/?0=" . $keyValue;
			    $response_pk = file_get_contents($url_pk);
			    $data = json_decode($response_pk, true);
			    
			    if ($response !== false && $response != "0")
			    {
			        if ($data['dev'] === true || $data['pinkeyed'] === true)
			        {
			            $found_key = true;
			        }
			    }
			    
                if ($found_key)
                {
                    header('HTTP/1.1 200 OK');
                    header('Content-Type: text/html');
                    header('Stand-User-Type: ' . $response);
                    exit();
                }
                else
    			{
                    header('HTTP/1.1 404 Not Found');
                    header('Content-Type: text/html');
                    exit();
    			}
			}
			else
			{
                header('HTTP/1.1 404 Not Found');
                header('Content-Type: text/html');
                exit();
			}
		}
    ?>